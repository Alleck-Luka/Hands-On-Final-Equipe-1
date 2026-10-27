#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Alleck dos Santos <allecklukap@gmail.com>");
MODULE_DESCRIPTION("Driver de acesso ao LoRaComm (ESP32 com Chip Serial CP2102)");
MODULE_LICENSE("GPL");

#define MAX_RECV_LINE 256
#define USB_TIMEOUT_MS 5000
#define CP210X_REQTYPE_HOST_TO_INTERFACE \
    (USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE)

#define CP210X_IFC_ENABLE       0x00
#define CP210X_SET_LINE_CTL     0x03
#define CP210X_SET_BAUDRATE     0x1E

#define CP210X_UART_ENABLE      0x0001
#define CP210X_UART_DISABLE     0x0000

#define CP210X_BITS_DATA_8      0x0800
#define CP210X_PARITY_NONE      0x0000
#define CP210X_STOP_BITS_1      0x0000

#define LORACOMM_BAUDRATE      115200
#define CP210X_SET_MHS       0x07
#define CP210X_PURGE         0x12
#define CP210X_SET_FLOW      0x13

#define CONTROL_DTR          0x0001
#define CONTROL_RTS          0x0002
#define CONTROL_WRITE_DTR    0x0100
#define CONTROL_WRITE_RTS    0x0200

#define PURGE_ALL            0x000f

#define CP210X_SERIAL_DTR_ACTIVE 1
#define CP210X_SERIAL_RTS_ACTIVE 1

struct cp210x_flow_ctl {
    __le32 ulControlHandshake;
    __le32 ulFlowReplace;
    __le32 ulXonLimit;
    __le32 ulXoffLimit;
} __packed;

static int  usb_probe(struct usb_interface *ifce, const struct usb_device_id *id);
static void usb_disconnect(struct usb_interface *ifce);
static int usb_send_cmd(char *cmd, char *param, char *response, size_t response_size);
static ssize_t attr_show(struct kobject *sys_obj, struct kobj_attribute *attr, char *buff);
static ssize_t attr_store(struct kobject *sys_obj, struct kobj_attribute *attr, const char *buff, size_t count);
static int cp210x_write_u16(struct usb_interface *interface,
                            u8 request,
                            u16 value);
static int cp210x_write_u32(struct usb_interface *interface,
                            u8 request,
                            u32 value);
static int cp210x_init_uart(struct usb_interface *interface);
static int loracomm_reader(void *data);
static void loracomm_dispatch_line(char *line);
static ssize_t loracomm_evt_read(struct file *file, char __user *buf,
                                 size_t len, loff_t *off);
static unsigned int loracomm_evt_poll(struct file *file, poll_table *wait);

static char recv_line[MAX_RECV_LINE];
static struct usb_device *loracomm_device;
static uint usb_in, usb_out;
static char *usb_in_buffer, *usb_out_buffer;
static int usb_max_size;

/*
 * ============================================================================
 *  NÍVEL 2 - evento real (sem polling)
 *
 *  O firmware já emite "EVT RX <payload>" na serial quando uma mensagem LoRa
 *  chega. Aqui o driver passa a ENTREGAR isso como evento:
 *
 *    - um kthread (loracomm_reader) é o ÚNICO leitor do bulk-IN; ele monta as
 *      linhas e despacha:
 *         "EVT RX <payload>" -> guarda em evt_payload e wake_up(&evt_wq)
 *         "RES <cmd> ..."    -> completa o comando pendente (cmd_done)
 *    - /dev/loracomm (misc device) expõe read()/poll() para o userspace
 *      (o LoRa HAL e o Sensors HAL) bloquear até chegar mensagem.
 * ============================================================================
 */

/* ---- canal de evento (EVT RX) ---- */
static DECLARE_WAIT_QUEUE_HEAD(evt_wq);
static char evt_payload[MAX_RECV_LINE];
static bool evt_available;
static bool evt_shutdown;
static DEFINE_MUTEX(evt_lock);

/* ---- resposta de comando (RES ...) ---- */
static char cmd_expected[MAX_RECV_LINE];   /* ex.: "RES GET_RX_COUNT" */
static char cmd_response[MAX_RECV_LINE];   /* valor extraido (ex.: "5") */
static bool cmd_pending;
static DEFINE_MUTEX(cmd_lock);             /* protege cmd_* */
static DEFINE_MUTEX(cmd_serial_lock);      /* um comando por vez */
static DECLARE_COMPLETION(cmd_done);

/* ---- kthread leitor + /dev/loracomm ---- */
static struct task_struct *reader_task;

// Atributos que irão para o sysf
// Explicação: _ATTR(nome, permissões, função_show, função_store)
//             função_show é chamada quando o usuário lê o arquivo (vide cat ldr)
//             função_store é chamada quando o usuário escreve no arquivo (vide echo 1 > led)

static struct kobj_attribute send_attribute = __ATTR(send, S_IWUSR, NULL, attr_store);
static struct kobj_attribute ping_attribute = __ATTR(ping, S_IRUGO, attr_show, NULL);
static struct kobj_attribute rx_count_attribute = __ATTR(rx_count, S_IRUGO, attr_show, NULL);
static struct kobj_attribute last_rx_attribute = __ATTR(last_rx, S_IRUGO, attr_show, NULL);
static struct kobj_attribute aux_attribute = __ATTR(aux, S_IRUGO, attr_show, NULL);
static struct kobj_attribute key_attribute = __ATTR(key, S_IRUGO | S_IWUSR, attr_show, attr_store);

static struct attribute      *attrs[]       = { &send_attribute.attr, &ping_attribute.attr, &rx_count_attribute.attr, &last_rx_attribute.attr, &aux_attribute.attr, &key_attribute.attr, NULL };
static struct attribute_group attr_group    = { .attrs = attrs };
static struct kobject        *sys_obj;

#define VENDOR_ID  0x10C4
#define PRODUCT_ID 0xEA60
static const struct usb_device_id id_table[] = { { USB_DEVICE(VENDOR_ID, PRODUCT_ID) }, {} };
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver loracomm_driver = {
    .name        = "loracomm",
    .probe       = usb_probe,
    .disconnect  = usb_disconnect,
    .id_table    = id_table,
};
module_usb_driver(loracomm_driver);

static int cp210x_write_block(struct usb_interface *interface,
                              u8 request,
                              void *buf,
                              int size)
{
    struct usb_device *udev;
    u16 interface_number;
    void *dmabuf;
    int ret;

    udev = interface_to_usbdev(interface);

    interface_number =
        interface->cur_altsetting->desc.bInterfaceNumber;

    dmabuf = kmemdup(buf, size, GFP_KERNEL);
    if (!dmabuf)
        return -ENOMEM;

    ret = usb_control_msg(
        udev,
        usb_sndctrlpipe(udev, 0),
        request,
        USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
        0,
        interface_number,
        dmabuf,
        size,
        USB_CTRL_SET_TIMEOUT
    );

    kfree(dmabuf);

    if (ret < 0)
        return ret;

    if (ret != size)
        return -EIO;

    return 0;
}

static int usb_probe(struct usb_interface *interface,
                     const struct usb_device_id *id)
{
    struct usb_endpoint_descriptor *usb_endpoint_in;
    struct usb_endpoint_descriptor *usb_endpoint_out;
    int ret;

    printk(KERN_INFO "LoRaComm: === DISPOSITIVO CONECTADO ===\n");

    /*
     * 1. Primeiro obtenha e retenha o dispositivo USB.
     */
    loracomm_device = usb_get_dev(interface_to_usbdev(interface));
    usb_set_intfdata(interface, loracomm_device);

    printk(KERN_INFO
           "LoRaComm: VendorID=0x%04x, ProductID=0x%04x\n",
           le16_to_cpu(loracomm_device->descriptor.idVendor),
           le16_to_cpu(loracomm_device->descriptor.idProduct));

    /*
     * 2. Descubra os endpoints ANTES de publicar o sysfs.
     */
    ret = usb_find_common_endpoints(
        interface->cur_altsetting,
        &usb_endpoint_in,
        &usb_endpoint_out,
        NULL,
        NULL
    );

    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Erro ao encontrar endpoints USB (ret=%d)\n",
               ret);
        goto err_device;
    }

    usb_in = usb_endpoint_in->bEndpointAddress;
    usb_out = usb_endpoint_out->bEndpointAddress;

    usb_max_size = max(
        usb_endpoint_maxp(usb_endpoint_in),
        usb_endpoint_maxp(usb_endpoint_out)
    );

    printk(KERN_INFO
           "LoRaComm: Endpoints IN=0x%02x OUT=0x%02x MaxSize=%d\n",
           usb_in,
           usb_out,
           usb_max_size);

    /*
     * 3. Aloca buffers.
     */
    usb_in_buffer = kmalloc(usb_max_size, GFP_KERNEL);
    usb_out_buffer = kmalloc(usb_max_size, GFP_KERNEL);

    if (!usb_in_buffer || !usb_out_buffer) {
        printk(KERN_ERR "LoRaComm: Falha ao alocar buffers\n");
        ret = -ENOMEM;
        goto err_buffers;
    }

    ret = cp210x_init_uart(interface);

    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Inicialização do CP2102 falhou: %d\n",
               ret);
        goto err_buffers;
    }

    /*
     * 4. Só agora exponha o dispositivo para userspace.
     */
    sys_obj = kobject_create_and_add("loracomm", kernel_kobj);

    if (!sys_obj) {
        printk(KERN_ERR "LoRaComm: Falha ao criar kobject\n");
        ret = -ENOMEM;
        goto err_buffers;
    }

    ret = sysfs_create_group(sys_obj, &attr_group);

    if (ret) {
        printk(KERN_ERR "LoRaComm: Falha ao criar sysfs group\n");
        goto err_kobject;
    }

    printk(KERN_INFO
           "LoRaComm: Sysfs criado em /sys/kernel/loracomm/\n");

    /*
     * 5. Nivel 2: cria o /dev/loracomm (canal de eventos) e o kthread leitor.
     */
    evt_available = false;
    evt_shutdown = false;

    ret = misc_register(&loracomm_misc);
    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Falha ao registrar /dev/loracomm: %d\n", ret);
        goto err_sysfs;
    }

    reader_task = kthread_run(loracomm_reader, NULL, "loracomm_rx");
    if (IS_ERR(reader_task)) {
        ret = PTR_ERR(reader_task);
        reader_task = NULL;
        printk(KERN_ERR
               "LoRaComm: Falha ao criar kthread leitor: %d\n", ret);
        goto err_misc;
    }

    printk(KERN_INFO
           "LoRaComm: /dev/loracomm criado (eventos EVT RX)\n");

    printk(KERN_INFO
           "LoRaComm: === PROBE CONCLUÍDO COM SUCESSO ===\n");

    return 0;

err_misc:
    misc_deregister(&loracomm_misc);

err_sysfs:
    sysfs_remove_group(sys_obj, &attr_group);

err_kobject:
    kobject_put(sys_obj);
    sys_obj = NULL;

err_buffers:
    kfree(usb_in_buffer);
    kfree(usb_out_buffer);

    usb_in_buffer = NULL;
    usb_out_buffer = NULL;

err_device:
    usb_set_intfdata(interface, NULL);

    if (loracomm_device) {
        usb_put_dev(loracomm_device);
        loracomm_device = NULL;
    }

    return ret;
}

static int cp210x_write_u16(struct usb_interface *interface,
                            u8 request,
                            u16 value)
{
    struct usb_device *udev;
    u16 interface_number;
    int ret;

    udev = interface_to_usbdev(interface);
    interface_number =
        interface->cur_altsetting->desc.bInterfaceNumber;

    ret = usb_control_msg(
        udev,
        usb_sndctrlpipe(udev, 0),
        request,
        CP210X_REQTYPE_HOST_TO_INTERFACE,
        value,
        interface_number,
        NULL,
        0,
        USB_CTRL_SET_TIMEOUT
    );

    if (ret < 0) {
        printk(KERN_ERR
               "LoRaComm: CP2102 control request 0x%02x falhou: %d\n",
               request, ret);
        return ret;
    }

    return 0;
}

static int cp210x_write_u32(struct usb_interface *interface,
                            u8 request,
                            u32 value)
{
    struct usb_device *udev;
    __le32 *buffer;
    u16 interface_number;
    int ret;

    udev = interface_to_usbdev(interface);
    interface_number =
        interface->cur_altsetting->desc.bInterfaceNumber;

    buffer = kmalloc(sizeof(*buffer), GFP_KERNEL);
    if (!buffer)
        return -ENOMEM;

    *buffer = cpu_to_le32(value);

    ret = usb_control_msg(
        udev,
        usb_sndctrlpipe(udev, 0),
        request,
        CP210X_REQTYPE_HOST_TO_INTERFACE,
        0,
        interface_number,
        buffer,
        sizeof(*buffer),
        USB_CTRL_SET_TIMEOUT
    );

    kfree(buffer);

    if (ret < 0) {
        printk(KERN_ERR
               "LoRaComm: CP2102 control request 0x%02x falhou: %d\n",
               request, ret);
        return ret;
    }

    if (ret != sizeof(__le32))
        return -EIO;

    return 0;
}

static int cp210x_init_uart(struct usb_interface *interface)
{
    int ret;
    u16 line_ctl;

    printk(KERN_INFO "LoRaComm: Inicializando UART CP2102...\n");

    /*
     * 1. Habilita a UART.
     */
    ret = cp210x_write_u16(
        interface,
        CP210X_IFC_ENABLE,
        CP210X_UART_ENABLE
    );

    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Falha ao habilitar UART CP2102: %d\n",
               ret);
        return ret;
    }

    printk(KERN_INFO "LoRaComm: UART CP2102 habilitada\n");

    /*
     * 2. Baud rate = 115200.
     */
    ret = cp210x_write_u32(
        interface,
        CP210X_SET_BAUDRATE,
        LORACOMM_BAUDRATE
    );

    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Falha ao configurar baud rate: %d\n",
               ret);
        return ret;
    }

    printk(KERN_INFO
           "LoRaComm: Baud rate configurado para %d\n",
           LORACOMM_BAUDRATE);

    /*
     * 3. 8 bits, sem paridade, 1 stop bit = 8N1.
     */
    line_ctl =
        CP210X_BITS_DATA_8 |
        CP210X_PARITY_NONE |
        CP210X_STOP_BITS_1;

    ret = cp210x_write_u16(
        interface,
        CP210X_SET_LINE_CTL,
        line_ctl
    );

    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Falha ao configurar 8N1: %d\n",
               ret);
        return ret;
    }

    printk(KERN_INFO "LoRaComm: UART configurada em 115200 8N1\n");

    struct cp210x_flow_ctl flow;
    memset(&flow, 0, sizeof(flow));

    flow.ulControlHandshake =
        cpu_to_le32(CP210X_SERIAL_DTR_ACTIVE);

    flow.ulFlowReplace =
        cpu_to_le32(CP210X_SERIAL_RTS_ACTIVE << 6);

        ret = cp210x_write_block(
            interface,
            CP210X_SET_FLOW,
            &flow,
            sizeof(flow)
        );

        if (ret) {
            printk(KERN_ERR
               "LoRaComm: Falha configurando flow control: %d\n",
               ret);
            return ret;
        }

        printk(KERN_INFO
           "LoRaComm: Flow control desabilitado\n");

        ret = cp210x_write_u16(
        interface,
        CP210X_SET_MHS,
        CONTROL_DTR |
        CONTROL_RTS |
        CONTROL_WRITE_DTR |
        CONTROL_WRITE_RTS
    );

    if (ret) {
        printk(KERN_ERR
               "LoRaComm: Falha configurando DTR/RTS: %d\n",
               ret);
        return ret;
    }

    printk(KERN_INFO
       "LoRaComm: DTR/RTS configurados\n");

    return 0;
}

static void usb_disconnect(struct usb_interface *interface)
{
    struct usb_device *udev;

    printk(KERN_INFO
           "LoRaComm: === DISPOSITIVO DESCONECTADO ===\n");

    udev = usb_get_intfdata(interface);
    usb_set_intfdata(interface, NULL);

    /*
     * Nivel 2: para o kthread leitor PRIMEIRO (e ele que usa usb_in_buffer)
     * e depois derruba o /dev/loracomm, soltando quem estiver em poll().
     */
    if (reader_task) {
        kthread_stop(reader_task);
        reader_task = NULL;
    }

    evt_shutdown = true;
    wake_up_interruptible(&evt_wq);

    misc_deregister(&loracomm_misc);

    /*
     * Primeiro retire a interface visível ao userspace,
     * para evitar novas operações.
     */
    if (sys_obj) {
        sysfs_remove_group(sys_obj, &attr_group);
        kobject_put(sys_obj);
        sys_obj = NULL;
    }

    kfree(usb_in_buffer);
    kfree(usb_out_buffer);

    usb_in_buffer = NULL;
    usb_out_buffer = NULL;

    loracomm_device = NULL;

    if (udev)
        usb_put_dev(udev);
}

/*
 * Classifica cada linha vinda do dispositivo:
 *  - "EVT RX <payload>" -> evento: guarda e acorda quem esta em poll()/read()
 *  - "RES <cmd> ..."    -> resposta: completa o comando pendente
 *  - resto              -> descarta
 */
static void loracomm_dispatch_line(char *line)
{
    if (!strncmp(line, "EVT RX ", 7)) {
        mutex_lock(&evt_lock);
        strscpy(evt_payload, line + 7, sizeof(evt_payload));
        evt_available = true;
        mutex_unlock(&evt_lock);

        wake_up_interruptible(&evt_wq);

        printk(KERN_INFO "LoRaComm: EVT RX -> '%s'\n", evt_payload);
        return;
    }

    if (!strncmp(line, "RES ", 4)) {
        mutex_lock(&cmd_lock);
        if (cmd_pending &&
            !strncmp(line, cmd_expected, strlen(cmd_expected))) {
            char *val = line + strlen(cmd_expected);

            if (*val == ' ')
                val++;

            strscpy(cmd_response, val, sizeof(cmd_response));
            complete(&cmd_done);
        }
        mutex_unlock(&cmd_lock);
        return;
    }
}

/*
 * kthread: UNICO leitor do bulk-IN. Drena a USB continuamente, monta linhas
 * e despacha (evento x resposta). Substitui a leitura sincrona que o
 * usb_send_cmd fazia antes.
 */
static int loracomm_reader(void *data)
{
    int actual_size, i;
    int recv_size = 0;

    while (!kthread_should_stop()) {
        int ret;

        if (!loracomm_device)
            break;

        ret = usb_bulk_msg(
            loracomm_device,
            usb_rcvbulkpipe(loracomm_device, usb_in),
            usb_in_buffer,
            min(usb_max_size, MAX_RECV_LINE),
            &actual_size,
            2000
        );

        if (ret || actual_size <= 0)
            continue;

        for (i = 0; i < actual_size; i++) {
            char c = usb_in_buffer[i];

            if (c == '\n') {
                recv_line[recv_size] = '\0';
                if (recv_size > 0)
                    loracomm_dispatch_line(recv_line);
                recv_size = 0;
            } else if (c != '\r' && recv_size < MAX_RECV_LINE - 1) {
                recv_line[recv_size++] = c;
            }
        }
    }

    return 0;
}

/* ---- /dev/loracomm: canal de eventos (EVT RX) com read()/poll() ---- */

static ssize_t loracomm_evt_read(struct file *file, char __user *buf,
                                 size_t len, loff_t *off)
{
    int n;

    if (wait_event_interruptible(evt_wq, evt_available || evt_shutdown))
        return -ERESTARTSYS;

    if (evt_shutdown && !evt_available)
        return -ENODEV;

    mutex_lock(&evt_lock);
    n = strlen(evt_payload);
    if ((size_t)n > len)
        n = len;
    if (copy_to_user(buf, evt_payload, n)) {
        mutex_unlock(&evt_lock);
        return -EFAULT;
    }
    evt_available = false;
    mutex_unlock(&evt_lock);

    return n;
}

static unsigned int loracomm_evt_poll(struct file *file, poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &evt_wq, wait);

    mutex_lock(&evt_lock);
    if (evt_available)
        mask |= POLLIN | POLLRDNORM;
    mutex_unlock(&evt_lock);

    if (evt_shutdown)
        mask |= POLLHUP;

    return mask;
}

static const struct file_operations loracomm_fops = {
    .owner = THIS_MODULE,
    .read  = loracomm_evt_read,
    .poll  = loracomm_evt_poll,
};

static struct miscdevice loracomm_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "loracomm",
    .fops  = &loracomm_fops,
};

static int usb_send_cmd(char *cmd, char *param, char *response, size_t response_size) {
    int ret, actual_size;
    long waited;

    printk(KERN_INFO "LoRaComm: === ENVIANDO COMANDO ===\n");
    printk(KERN_INFO "LoRaComm: Comando: %s, Param: %s\n", cmd, param);

    if (!loracomm_device) {
        printk(KERN_ERR "LoRaComm: Dispositivo não conectado\n");
        return -1;
    }

    /* Nivel 2: um comando por vez (a leitura e feita pelo kthread) */
    mutex_lock(&cmd_serial_lock);

    // Monta: SEND mensagem\n ou PING\n
    if (param)
        snprintf(usb_out_buffer, usb_max_size,
                 "%s %s\n", cmd, param);
    else
        snprintf(usb_out_buffer, usb_max_size,
                 "%s\n", cmd);

    /*
     * Prepara o que esperamos ANTES de enviar, para o reader (que roda em
     * paralelo) nao perder a resposta.
     */
    mutex_lock(&cmd_lock);
    snprintf(cmd_expected, sizeof(cmd_expected), "RES %s", cmd);
    cmd_response[0] = '\0';
    cmd_pending = true;
    mutex_unlock(&cmd_lock);
    reinit_completion(&cmd_done);

    printk(KERN_INFO "LoRaComm: Buffer de saida: '%s' (tamanho: %zu)\n",
           usb_out_buffer, strlen(usb_out_buffer));

    ret = usb_bulk_msg(
        loracomm_device,
        usb_sndbulkpipe(loracomm_device, usb_out),
        usb_out_buffer,
        strlen(usb_out_buffer),
        &actual_size,
        USB_TIMEOUT_MS
    );

    if (ret) {
        printk(KERN_ERR "LoRaComm: Erro ao enviar comando! Código: %d\n",
               ret);
        mutex_lock(&cmd_lock);
        cmd_pending = false;
        mutex_unlock(&cmd_lock);
        mutex_unlock(&cmd_serial_lock);
        return -1;
    }

    printk(KERN_INFO "LoRaComm: Comando enviado, %d bytes\n",
           actual_size);

    /*
     * Nivel 2: a leitura e feita pelo kthread (loracomm_reader). Aqui so
     * esperamos o completion que ele dispara ao ver "RES <cmd>".
     */
    printk(KERN_INFO "LoRaComm: Esperando resposta (completion)...\n");

    waited = wait_for_completion_interruptible_timeout(
        &cmd_done, msecs_to_jiffies(USB_TIMEOUT_MS));

    mutex_lock(&cmd_lock);
    if (waited > 0 && cmd_response[0] != '\0') {
        strscpy(response, cmd_response, response_size);
        cmd_pending = false;
        mutex_unlock(&cmd_lock);
        mutex_unlock(&cmd_serial_lock);

        printk(KERN_INFO "LoRaComm: Resposta aceita: '%s'\n", response);
        return 0;
    }
    cmd_pending = false;
    mutex_unlock(&cmd_lock);
    mutex_unlock(&cmd_serial_lock);

    printk(KERN_ERR
           "LoRaComm: TIMEOUT - sem resposta para %s\n",
           cmd);

    return -1;
}

static ssize_t attr_show(struct kobject *sys_obj,
                         struct kobj_attribute *attr,
                         char *buff)
{
    const char *attr_name = attr->attr.name;
    char response[MAX_RECV_LINE];
    int ret;

    printk(KERN_INFO "LoRaComm: === LENDO %s ===\n", attr_name);

    memset(response, 0, sizeof(response));

    if (!strcmp(attr_name, "rx_count")) {
        ret = usb_send_cmd("GET_RX_COUNT", NULL,
                           response, sizeof(response));

    } else if (!strcmp(attr_name, "last_rx")) {
        ret = usb_send_cmd("GET_LAST_RX", NULL,
                           response, sizeof(response));

    } else if (!strcmp(attr_name, "aux")) {
        ret = usb_send_cmd("GET_AUX", NULL,
                           response, sizeof(response));

    } else if (!strcmp(attr_name, "ping")) {
        ret = usb_send_cmd("PING", NULL,
                           response, sizeof(response));
    
    } else if (!strcmp(attr_name, "key")) {
        ret = usb_send_cmd("GET_KEY", NULL,
                            response, sizeof(response));

    } else {
        printk(KERN_WARNING
               "LoRaComm: atributo %s desconhecido\n",
               attr_name);
        return -EINVAL;
    }

    if (ret < 0) {
        printk(KERN_WARNING
               "LoRaComm: Erro ao ler %s\n",
               attr_name);
        return ret;
    }

    return scnprintf(buff, PAGE_SIZE, "%s\n", response);
}

static ssize_t attr_store(struct kobject *sys_obj,
                          struct kobj_attribute *attr,
                          const char *buff,
                          size_t count)
{
    const char *attr_name = attr->attr.name;
    char param[MAX_RECV_LINE];
    char response[MAX_RECV_LINE];
    int ret;

    if (count == 0)
        return -EINVAL;

    /*
     * O sysfs pode enviar o '\n' no final.
     * Copiamos para um buffer próprio para podermos
     * remover esse '\n'.
     */
    if (count >= sizeof(param))
        return -EINVAL;

    memcpy(param, buff, count);
    param[count] = '\0';

    if (param[count - 1] == '\n')
        param[count - 1] = '\0';

    memset(response, 0, sizeof(response));

    printk(KERN_INFO "LoRaComm: === SETANDO %s ===\n", attr_name);

    if (!strcmp(attr_name, "send")) {

        printk(KERN_INFO
               "LoRaComm: Enviando mensagem: '%s'\n",
               param);

        ret = usb_send_cmd("SEND", param,
                           response, sizeof(response));

    } else if (!strcmp(attr_name, "key")) {

        printk(KERN_INFO
               "LoRaComm: Setando chave: '%s'\n",
               param);

        ret = usb_send_cmd("SET_KEY", param,
                           response, sizeof(response));

    } else {
        printk(KERN_WARNING
               "LoRaComm: atributo %s nao pode ser escrito\n",
               attr_name);
        return -EACCES;
    }

    if (ret < 0) {
        printk(KERN_WARNING
               "LoRaComm: Erro ao executar %s\n",
               attr_name);
        return ret;
    }

    return count;
}