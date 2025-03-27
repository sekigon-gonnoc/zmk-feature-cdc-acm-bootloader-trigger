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

## Requirements

- At least one device compatible with `zephyr,cdc-acm-uart` must be available in your device tree. Without this, the build will fail.
- If you have ZMK Studio or USB Logging enabled, those CDC ACM devices can be automatically detected and used.

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

You can add the bootloader trigger in two ways:

#### Option 1: Auto-detection

```dts
/ {
    bootloader_trigger: bootloader_trigger {
        compatible = "zmk,cdc-acm-bootloader-trigger";
    };
};
```

With this configuration, the module will automatically detect and use the first available device with `zephyr,cdc-acm-uart` compatibility. If you have ZMK Studio or USB Logging enabled, those CDC ACM devices will be found and used.

#### Option 2: Specifying a particular CDC ACM UART port

If you have multiple CDC ACM ports and want to use a specific one, you can specify it with the `cdc-port` property:

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

Or using the ZMK USB logging UART port:

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

## 要件

- デバイスツリーに`zephyr,cdc-acm-uart`と互換性のあるデバイスが少なくとも1つ必要です。これがないとビルドは失敗します。
- ZMK StudioまたはUSBロギングが有効になっている場合、それらのCDC ACMデバイスを自動的に検出できます。

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

ブートローダートリガーを追加するには2つの方法があります：

#### 方法1: 自動検出

```dts
/ {
    bootloader_trigger: bootloader_trigger {
        compatible = "zmk,cdc-acm-bootloader-trigger";
    };
};
```

この設定では、モジュールは`zephyr,cdc-acm-uart`互換性を持つ最初に利用可能なデバイスを自動的に検出して使用します。ZMK StudioやUSBロギングが有効になっている場合、それらのCDC ACMデバイスが検出されて使用されます。

#### 方法2: 特定のCDC ACM UARTポートを指定

複数のCDC ACMポートがあり、特定のポートを使用したい場合は、`cdc-port`プロパティで指定できます：

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

または、ZMK USBロギングUARTポートを使用する場合：

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