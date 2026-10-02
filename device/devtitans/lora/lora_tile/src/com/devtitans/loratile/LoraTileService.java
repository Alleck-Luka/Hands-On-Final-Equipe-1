package com.devtitans.loratile;

import android.service.quicksettings.Tile;
import android.service.quicksettings.TileService;

public class LoraTileService extends TileService {

    private boolean enabled = false;

    @Override
    public void onStartListening() {
        super.onStartListening();
        updateTile();
    }

    @Override
    public void onClick() {
        super.onClick();

        enabled = !enabled;

        updateTile();
    }

    private void updateTile() {
        Tile tile = getQsTile();

        if (tile == null) {
            return;
        }

        if (enabled) {
            tile.setState(Tile.STATE_ACTIVE);
            tile.setLabel("LoRa ON");
        } else {
            tile.setState(Tile.STATE_INACTIVE);
            tile.setLabel("LoRa OFF");
        }

        tile.updateTile();
    }
}