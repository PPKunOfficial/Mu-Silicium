  FILE FREEFORM = A1E235DE-E825-4591-9623-C43175811826 {
    SECTION UI = "SecParti.cfg"
    SECTION RAW = Binaries/ovaltine/RawFiles/SecParti.cfg
  }

!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  # OnePlus Ace Pro uses a BOE RM692E5 panel (1080x2412 DSC cmd).
  FILE FREEFORM = B8AFC726-C0DD-4E5A-A72D-18004975591E {
    SECTION UI = "Panel_oplus_boe_rm692e5_1080_2412_dsc_cmd.xml"
    SECTION RAW = Binaries/ovaltine/RawFiles/Panel_oplus_boe_rm692e5_1080_2412_dsc_cmd.xml
  }

  FILE FREEFORM = 03E9256F-E621-48D8-8576-7E3B4FCF2D41 {
    SECTION UI = "PanelSimConfig.csv"
    SECTION RAW = Binaries/ovaltine/RawFiles/PanelSimConfig.csv
  }
!endif
