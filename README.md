# ZMK CDC ACM Bootloader Trigger

This feature enables your device to enter bootloader mode when a CDC ACM port is opened at 1200 baud rate. This is compatible with the Arduino bootloader trigger mechanism used by many tools like Arduino IDE, PlatformIO, etc.

## Installation

1. Add the module to your `zmk-config/config/west.yml` file:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: sekigon-gonnoc
      url-base: https://github.com/sekigon-gonnoc
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-feature-cdc-acm-bootloader-trigger
      remote: sekigon-gonnoc
      revision: main
  self:
    path: config
```

## Usage

### 1. Enable the feature in your <keyboard>.conf

Add the following line to your `<keyboard>.conf` file:

```
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER=y
```

Optionally, you can also configure additional settings:

```
# Delay before entering bootloader mode (default: 100ms)
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_DELAY_MS=100

# Polling interval for CDC ACM state (default: 100ms)
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_POLL_MS=100

# Reset code for entering bootloader mode (default: 0x57 for Adafruit nRF52 bootloader)
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_RESET_CODE=0x57
```

The `RESET_CODE` should be set to match your board's bootloader entry code:
- 0x57: Adafruit nRF52 bootloader
- Check your MCU or bootloader documentation for the appropriate value for other boards

### 2. Add the bootloader trigger node to your device tree overlay

There are two common configurations:

#### Option 1: Using a dedicated CDC ACM UART port

```dts
/ {
    bootloader_trigger: bootloader_trigger {
        compatible = "zmk,cdc-acm-bootloader-trigger";
        cdc-port = <&cdc_acm_uart0>;
    };
};

&zephyr_udc0 {
    cdc_acm_uart0: cdc_acm_uart0 {
        compatible = "zephyr,cdc-acm-uart";
    };
};
```

#### Option 2: Using the ZMK USB logging UART port

```dts
/ {
    bootloader_trigger: bootloader_trigger {
        compatible = "zmk,cdc-acm-bootloader-trigger";
        cdc-port = <&snippet_zmk_usb_logging_uart>;
    };
};
```

### 3. How to use

Once configured, your device will automatically enter bootloader mode when a compatible tool attempts to flash firmware by:
1. Opening the configured CDC ACM port at 1200 baud
2. Closing the port
3. The device will reset into bootloader mode after the configured delay

This is compatible with tools like Arduino IDE, PlatformIO, QMK Toolbox, and others that support this trigger mechanism.

---

# ZMK CDC ACM ブートローダートリガー

この機能を使用すると、CDC ACMポートが1200ボーレートで開かれたときに、デバイスがブートローダーモードに入るようになります。これはArduino IDEやPlatformIOなど多くのツールで使用されているArduinoブートローダートリガーの仕組みと互換性があります。

## インストール方法

1. モジュールを`zmk-config/config/west.yml`ファイルに追加します：

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: sekigon-gonnoc
      url-base: https://github.com/sekigon-gonnoc
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-feature-cdc-acm-bootloader-trigger
      remote: sekigon-gonnoc
      revision: main
  self:
    path: config
```

## 使用方法

### 1. <keyboard>.confで機能を有効にする

`<keyboard>.conf`ファイルに以下の行を追加してください：

```
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER=y
```

必要に応じて、追加の設定も構成できます：

```
# ブートローダーモードに入る前の遅延時間（デフォルト：100ms）
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_DELAY_MS=100

# CDC ACM状態のポーリング間隔（デフォルト：100ms）
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_POLL_MS=100

# ブートローダーモードに入るためのリセットコード（デフォルト：Adafruit nRF52ブートローダー用の0x57）
CONFIG_ZMK_CDC_ACM_BOOTLOADER_TRIGGER_RESET_CODE=0x57
```

`RESET_CODE`はボードのブートローダー入力コードに合わせて設定する必要があります：
- 0x57: Adafruit nRF52ブートローダー
- 他のボードでは、MCUまたはブートローダーのドキュメントで適切な値を確認してください

### 2. デバイスツリーオーバーレイにブートローダートリガーノードを追加

一般的な設定には2つの方法があります：

#### 方法1: 専用のCDC ACM UARTポートを使用

```dts
/ {
    bootloader_trigger: bootloader_trigger {
        compatible = "zmk,cdc-acm-bootloader-trigger";
        cdc-port = <&cdc_acm_uart0>;
    };
};

&zephyr_udc0 {
    cdc_acm_uart0: cdc_acm_uart0 {
        compatible = "zephyr,cdc-acm-uart";
    };
};
```

#### 方法2: ZMK USBロギングUARTポートを使用

```dts
/ {
    bootloader_trigger: bootloader_trigger {
        compatible = "zmk,cdc-acm-bootloader-trigger";
        cdc-port = <&snippet_zmk_usb_logging_uart>;
    };
};
```

### 3. 使用方法

設定が完了すると、互換性のあるツールがファームウェアをフラッシュしようとしたときに、デバイスは自動的にブートローダーモードに入ります：
1. 設定されたCDC ACMポートが1200ボーレートで開かれる
2. ポートが閉じられる
3. 設定された遅延時間後にデバイスがブートローダーモードでリセットされる

これはArduino IDE、PlatformIO、QMK Toolboxなど、このトリガー機構をサポートするツールと互換性があります。