  INF MdeModulePkg/Core/Dxe/DxeMain.inf
  INF MdeModulePkg/Universal/PCD/Dxe/Pcd.inf

  INF Binaries/ovaltine/QcomPkg/Drivers/EnvDxe/EnvDxe.inf

  INF MdeModulePkg/Universal/ReportStatusCodeRouter/RuntimeDxe/ReportStatusCodeRouterRuntimeDxe.inf
  INF MdeModulePkg/Universal/StatusCodeHandler/RuntimeDxe/StatusCodeHandlerRuntimeDxe.inf
  INF ArmPkg/Drivers/CpuDxe/CpuDxe.inf
  INF MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
  INF MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf

  INF Binaries/ovaltine/QcomPkg/Drivers/ShmBridgeDxe/ShmBridgeDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/ScmDxeLA/ScmDxeLA.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/TzDxeLA/TzDxeLA.inf

  INF MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
  INF MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
  INF MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf
  INF EmbeddedPkg/EmbeddedMonotonicCounter/EmbeddedMonotonicCounter.inf
  INF MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
  INF EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
  INF EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
  INF MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
  INF ArmPkg/Drivers/ArmGicDxe/ArmGicDxe.inf
  INF ArmPkg/Drivers/TimerDxe/TimerDxe.inf

  INF Binaries/ovaltine/QcomPkg/Drivers/ChipInfo/ChipInfo.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/IPCCDxe/IPCCDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/GlinkDxe/GlinkDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/PlatformInfoDxeDriver/PlatformInfoDxeDriver.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/DALSys/DALSys.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/HALIOMMU/HALIOMMU.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/HWIODxeDriver/HWIODxeDriver.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/I2C/I2C.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/DALTLMM/DALTLMM.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/PmicDxe/PmicDxe.inf
  INF MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
  INF MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
  INF MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
  INF FatPkg/EnhancedFatDxe/Fat.inf
  INF MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

  INF Binaries/ovaltine/QcomPkg/Drivers/UFSDxe/UFSDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/CmdDbDxe/CmdDbDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/PwrUtilsDxe/PwrUtilsDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/RpmhDxe/RpmhDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/NpaDxe/NpaDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/ULogDxe/ULogDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/VcsDxe/VcsDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/ClockDxe/ClockDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/SPMI/SPMI.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/ICBDxe/ICBDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/SmemDxe/SmemDxe.inf
  INF ovaltinePkg/Drivers/LinuxLoaderDxe/LinuxLoaderDxe.inf
  INF MdeModulePkg/Universal/BdsDxe/BdsDxe.inf

# dropped (no stock equivalent): INF Binaries/pong/QcomPkg/Drivers/NT_DeviceInfoDxe/NT_DeviceInfoDxe.inf
# dropped (no stock equivalent): #INF Binaries/pong/QcomPkg/Drivers/BootloaderLoggingDxe/BootloaderLoggingDxe.inf

  INF EmbeddedPkg/SimpleTextInOutSerial/SimpleTextInOutSerial.inf
  INF MdeModulePkg/Universal/PrintDxe/PrintDxe.inf
  INF MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
  INF MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
  INF MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf

!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  INF Binaries/ovaltine/QcomPkg/Drivers/CPRDxe/CPRDxe.inf
!endif
  INF Binaries/ovaltine/QcomPkg/Drivers/GpiDxe/GpiDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/SPI/SPI.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/SdccDxe/SdccDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/FeatureEnablerDxe/FeatureEnablerDxe.inf

!if $(USE_CUSTOM_DISPLAY_DRIVER) == 1
  INF Binaries/ovaltine/QcomPkg/Drivers/DisplayDxe/DisplayDxe.inf
  #INF QcomPkg/Drivers/DisplayReEnablerDxe/DisplayReEnablerDxe.inf
!else
  INF SiliciumPkg/Drivers/SimpleFbDxe/SimpleFbDxe.inf
!endif

  INF Binaries/ovaltine/QcomPkg/Drivers/ButtonsDxe/ButtonsDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/ChargerExDxe/ChargerExDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/PmicGlinkDxe/PmicGlinkDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/QcomChargerDxeLA/QcomChargerDxeLA.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/UsbPwrCtrlDxe/UsbPwrCtrlDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/TsensDxe/TsensDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/LimitsDxe/LimitsDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/DDRInfoDxe/DDRInfoDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/UsbfnDwc3Dxe/UsbfnDwc3Dxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/XhciPciEmulation/XhciPciEmulation.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/XhciDxe/XhciDxe.inf

  INF MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
  INF MdeModulePkg/Bus/Usb/UsbKbDxe/UsbKbDxe.inf
  INF MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

  INF Binaries/ovaltine/QcomPkg/Drivers/UsbMsdDxe/UsbMsdDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/UsbDeviceDxe/UsbDeviceDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/UsbConfigDxe/UsbConfigDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/UsbInitDxe/UsbInitDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/RngDxe/RngDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/PILDxe/PILDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/PILProxyDxe/PILProxyDxe.inf
  INF Binaries/ovaltine/QcomPkg/Drivers/SPSSDxe/SPSSDxe.inf
