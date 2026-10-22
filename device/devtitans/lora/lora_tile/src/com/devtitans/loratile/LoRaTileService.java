package com.devtitans.loratile;

import android.os.RemoteException;
import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;

import devtitans.loramanager.LoraManager;

public class LoRaTileService extends TileService {


    LoraManager loraManager = LoraManager.getInstance();

    @Override
    public void onStartListening() {
        super.onStartListening();
        updateTile();
    }

    @Override
    public void onClick() {
        super.onClick();

        try {
            int state = loraManager.getLoraState();

            if(state == LoraManager.LORA_STATE_ENABLED) {
                loraManager.setLoraState(LoraManager.LORA_STATE_DISABLED);
            } else if (state == LoraManager.LORA_STATE_DISABLED) {
                loraManager.setLoraState(LoraManager.LORA_STATE_ENABLED);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        updateTile();
    }

    private void updateTile() {
        Tile tile = getQsTile();

        if (tile == null) {
            return;
        }

        try {
            int state = loraManager.getLoraState();
            // Existem: STATE_ACTIVE, STATE_INACTIVE e STATE_UNAVAILABLE
            if (state == LoraManager.LORA_STATE_ENABLED){
                tile.setState(Tile.STATE_ACTIVE);
                tile.setLabel("LoRa ON");
            } else if(state == LoraManager.LORA_STATE_DISABLED) {
                tile.setState(Tile.STATE_INACTIVE);
                tile.setLabel("LoRa OFF");
            } else if(state == LoraManager.LORA_STATE_UNAVAILABLE) {
                tile.setState(Tile.STATE_UNAVAILABLE);
                tile.setLabel("LoRa UNAVAILABLE");
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        

        tile.updateTile();
    }
}