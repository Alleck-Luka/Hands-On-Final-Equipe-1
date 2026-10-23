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
            boolean isAwake = loraManager.isAwake();

            if(isAwake) {
                loraManager.sleep();
            } else if (!isAwake) {
                loraManager.wakeup();
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
            boolean isAwake = loraManager.isAwake();
            // Existem: STATE_ACTIVE, STATE_INACTIVE e STATE_UNAVAILABLE
            if (isAwake){
                tile.setState(Tile.STATE_ACTIVE);
                tile.setLabel("LoRa ON");
            } else if(!isAwake) {
                tile.setState(Tile.STATE_INACTIVE);
                tile.setLabel("LoRa OFF");
            }
        } catch (RemoteException e) {
            tile.setState(Tile.STATE_UNAVAILABLE);
            tile.setLabel("LoRa UNAVAILABLE");
            e.printStackTrace();
        }
        

        tile.updateTile();
    }
}