package devtitans.lora;

@VintfStability
interface ILoraCallback {
    oneway void onMessageReceived(in String message);
}
