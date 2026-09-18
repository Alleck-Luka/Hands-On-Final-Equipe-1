#include <linux/module.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/delay.h>

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

static char recv_line[MAX_RECV_LINE];
static struct usb_device *loracomm_device;
static uint usb_in, usb_out;
static char *usb_in_buffer, *usb_out_buffer;
static int usb_max_size;

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

    printk(KERN_INFO
           "LoRaComm: === PROBE CONCLUÍDO COM SUCESSO ===\n");

    return 0;

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

static int usb_send_cmd(char *cmd, char *param, char *response, size_t response_size) {
    int recv_size = 0;
    int ret, actual_size, i;
    unsigned long timeout;
    char resp_expected[MAX_RECV_LINE];
    char *resp_pos;

    printk(KERN_INFO "LoRaComm: === ENVIANDO COMANDO ===\n");
    printk(KERN_INFO "LoRaComm: Comando: %s, Param: %s\n", cmd, param);

    if (!loracomm_device) {
        printk(KERN_ERR "LoRaComm: Dispositivo não conectado\n");
        return -1;
    }

    // Monta: SEND mensagem\n ou PING\n
    if (param)
        snprintf(usb_out_buffer, usb_max_size,
                 "%s %s\n", cmd, param);
    else
        snprintf(usb_out_buffer, usb_max_size,
                 "%s\n", cmd);

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
        return -1;
    }

    printk(KERN_INFO "LoRaComm: Comando enviado, %d bytes\n",
           actual_size);

    /*
     * Esperamos algo como:
     *
     * RES PING PONG
     * RES GET_RX_COUNT 5
     * RES GET_LAST_RX mensagem
     */
    snprintf(resp_expected, sizeof(resp_expected),
             "RES %s", cmd);

    printk(KERN_INFO "LoRaComm: Esperando resposta: '%s'\n",
           resp_expected);

    timeout = jiffies + msecs_to_jiffies(2000);

    while (time_before(jiffies, timeout)) {

        printk(KERN_INFO "LoRaComm: Lendo USB... \n");

        ret = usb_bulk_msg(
            loracomm_device,
            usb_rcvbulkpipe(loracomm_device, usb_in),
            usb_in_buffer,
            min(usb_max_size, MAX_RECV_LINE),
            &actual_size,
            2000
        );

        printk(KERN_INFO "LoRaComm: USB IN -> ret=%d, actual_size=%d\n", ret, actual_size);
        if (actual_size > 0) {
            printk(KERN_INFO "LoRaComm: Dados recebidos: '%.*s'\n", actual_size, usb_in_buffer);
        }

        if (ret) {
            printk(KERN_ERR "LoRaComm: Erro ao ler USB Código: %d\n", ret);

            continue;
        }

        if (actual_size == 0) {
            continue;
        }


        // Procura uma linha terminada por '\n'
        for (i = 0; i < actual_size; i++) {

            if (usb_in_buffer[i] == '\n') {

                recv_line[recv_size] = '\0';

                printk(KERN_INFO
                       "LoRaComm: Linha completa: '%s'\n",
                       recv_line);

                // bate o prefixo da resposta esperada
                if (!strncmp(recv_line,
                             resp_expected,
                             strlen(resp_expected))) {

                    // pula pra resposta
                    resp_pos = recv_line +
                               strlen(resp_expected);

                    if (*resp_pos == ' ')
                        resp_pos++;

                    // copia conteudo da resposta
                    strscpy(response,
                            resp_pos,
                            response_size);

                    printk(KERN_INFO
                           "LoRaComm: Resposta aceita: '%s'\n",
                           response);

                    return 0;
                }

                /*
                 * Linha não era a resposta esperada.
                 */
                recv_size = 0;

            } else if (recv_size < MAX_RECV_LINE - 1) {

                recv_line[recv_size++] =
                    usb_in_buffer[i];
            }
        }

    }

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