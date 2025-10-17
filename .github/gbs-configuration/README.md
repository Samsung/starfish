# gbs-configuration

This repository contains GBS configurations for SR Tizen GBS build.

Here is the profile list.

- Robot-related
  - /rbt/gems.conf : configuration for EX1 Project
  - /rbt/gbs\_botfit.conf : configuration for Botfit-Pro Project
- SR
  - tizen\_4.0.conf
    - profile.tizen\_4.0\_standard 
    - profile.tizen\_4.0\_emulator
  
  - tizen\_5.0.conf
    - profile.tizen\_5.0\_standard
    - profile.tizen\_5.0\_emulator
  
  - tizen\_5.5.conf
    - profile.tizen\_5.5\_standard
    - profile.tizen\_5.5\_emulator
  
  - tizen\_6.0.conf
    - profile.tizen\_6.0\_standard
    - profile.tizen\_6.0\_emulator

  - tizen\_6.5.conf
    - profile.tizen\_6.5\_standard
    - profile.tizen\_6.5\_emulator

  - tizen\_speaker.conf
    - profile.tizen\_5.5\_speaker
- DA
  - Tizen 6.0
    - DA Unified
      - Analyzer   : svace-3.0-20191202-gbs
      - Policy     : *
      - GBS Conf   : da/TIZEN_6.0_DA_UNIFIED_GBS.conf
      - GBS Profile: profile.da_unified_meson64_aarch64
      - Build Conf :
        - da/TIZEN_6.0_DA_UNIFIED_MESON64_BUILD.conf (for S922x, A311D)
        - da/TIZEN_6.0_DA_UNIFIED_R18_BUILD.conf (for R18)
      - Build Root : -
      - Manager    : DA Official (http://10.91.254.48:12000)
    - Family Hub
      - Analyzer   : svace-3.0-20191202-gbs
      - Policy     : *
      - GBS Conf   : da/TIZEN_6.0_DA_FHUB_GBS.conf
      - GBS Profile: profile.da_fhub_meson64_arm
      - Build Conf :
        - da/TIZEN_6.0_DA_FHUB_KANTM_BUILD.conf (for KantM)
        - da/TIZEN_6.0_DA_FHUB_MESON64_BUILD.conf (for S922x, A311D)
      - Build Root : -
      - Manager    : DA Official (http://10.91.254.48:12000)
  - Tizen 6.5
    - DA Robot
       - Analyzer   : svace-3.1-20210824-x64-linux-gbs
      - Policy     : *
      - GBS Conf   : da/TIZEN_6.5_DA_ROBOT_GBS.conf
      - GBS Profile:
        - profile.da_robot_qrb4210_aarch64 (for QRB4210)
        - profile.da_robot_r18_aarch64 (for R18)
      - Build Conf :
        - da/TIZEN_6.5_DA_ROBOT_QRB4210_BUILD.conf (for QRB4210)
        - da/TIZEN_6.5_DA_ROBOT_R18_BUILD.conf (for R18)
      - Build Root : -
      - Manager    : DA Official (http://10.91.254.48:12000)
- VD
  - Tizen 6.0 OneMAIN (TIZEN_ONEMAIN.gbs.conf)
    - New for 2021 products
      - profile.OscarP
      - profile.NikeM2
      - profile.NikeL
      - profile.KantSU2e
      - profile.OscarA
    - For 2020 products
      - profile.NikeM
      - profile.NikeM_WALL
      - profile.NikeL
      - profile.KantSU2
    - For 2019 products
      - profile.MuseM
      - profile.MuseL
    - LICENSING
      - profile.NVT_LICENSE
      - profile.MTK_LICENSE
      - profile.SLSI_LICENSE
    - SDK
      - profile.emulator32
