## 手机版Zcrazy（安卓）

- 安装包位置：`apk/zcrazy_android-debug.apk`

## 运行方式（App 内）

- 点击 `Start`：允许处理单播详细数据（并在点击车辆后发送订阅）。
- 点击车辆：开始接收/显示该车的详细面板。
- 点击 `Stop`：停止单播详情处理（仍显示组播可见的基础字段，例如红外/电池等），避免“抢主控”。
- 注意：正式比赛以及跑车的时候不能点击Start！！！
## 构建（Windows + PowerShell）

```powershell

# 安装到设备
adb install -r out\build\android-qt-arm64\android-build\build\outputs\apk\debug\android-build-debug.apk


（或者通过微信复制apk到手机上，用浏览器直接安装）
```

## 目录说明

- `src/`：C++ 逻辑（网络接收、模型、命令发送等）
- `qml/`：监控界面
- `android/`：Android 相关配置/资源
- `apk/`：用于发布/上传的 APK 文件
