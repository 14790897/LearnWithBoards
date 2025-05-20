document.addEventListener('DOMContentLoaded', function() {
    // 获取元素
    const ledToggle = document.getElementById('ledToggle');
    const ledStatus = document.getElementById('ledStatus');
    const brightnessSlider = document.getElementById('brightness');
    const brightnessValue = document.getElementById('brightnessValue');
    const saveSettingsBtn = document.getElementById('saveSettings');
    const restartDeviceBtn = document.getElementById('restartDevice');
    const motorSpeedSlider = document.getElementById('motorSpeed');
    const motorSpeedValue = document.getElementById('motorSpeedValue');
    const ipAddressElement = document.getElementById('ipAddress');
    const hostnameDisplayElement = document.getElementById('hostnameDisplay');

    // 初始化 - 获取 LED 状态和设备配置
    fetchLEDState();
    fetchDeviceConfig();

    // LED 开关事件监听
    ledToggle.addEventListener('change', function() {
        updateLED(this.checked, parseInt(brightnessSlider.value));
    });

    // 亮度滑块事件监听
    brightnessSlider.addEventListener('input', function() {
        brightnessValue.textContent = this.value;
        if (ledToggle.checked) {
            updateLED(true, parseInt(this.value));
        }
    });

    // 电机速度滑块事件监听
    if (motorSpeedSlider) {
        motorSpeedSlider.addEventListener('input', function() {
            if (motorSpeedValue) {
                motorSpeedValue.textContent = this.value;
            }
        });
    }

    // 重启设备按钮事件监听
    if (restartDeviceBtn) {
        restartDeviceBtn.addEventListener('click', function() {
            if (confirm('确定要重启设备吗？')) {
                restartDevice();
            }
        });
    }

    // 保存设置按钮事件监听
    saveSettingsBtn.addEventListener('click', function() {
        const hostname = document.getElementById('hostname').value;
        const wifiSSID = document.getElementById('wifiSSID').value;
        const wifiPassword = document.getElementById('wifiPassword').value;
        const staticIP = document.getElementById('staticIP')?.value;
        const gateway = document.getElementById('gateway')?.value;
        const subnet = document.getElementById('subnet')?.value;
        const motorSpeed = document.getElementById('motorSpeed')?.value;

        // 创建表单数据
        const formData = new FormData();
        if (hostname) formData.append('hostname', hostname);
        if (wifiSSID) formData.append('wifi_ssid', wifiSSID);
        if (wifiPassword) formData.append('wifi_password', wifiPassword);
        if (staticIP) formData.append('static_ip', staticIP);
        if (gateway) formData.append('gateway', gateway);
        if (subnet) formData.append('subnet', subnet);
        if (motorSpeed) formData.append('motor_speed', motorSpeed);

        // 发送请求保存设置
        fetch('/api/config', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                alert(data.message);

                // 如果需要重启，询问用户
                if (data.restart_required) {
                    if (confirm('需要重启设备才能应用新设置。\n\n是否立即重启？')) {
                        restartDevice();
                    }
                }

                // 重新获取设备配置
                fetchDeviceConfig();
            } else {
                alert('保存失败: ' + data.message);
            }
        })
        .catch(error => {
            console.error('保存设置失败:', error);
            alert('保存设置失败，请检查控制台了解详情。');
        });
    });

    // 获取 LED 状态
    function fetchLEDState() {
        fetch('/api/led')
            .then(response => response.json())
            .then(data => {
                ledToggle.checked = data.state;
                brightnessSlider.value = data.brightness;
                brightnessValue.textContent = data.brightness;
                ledStatus.textContent = data.state ? '开启' : '关闭';
            })
            .catch(error => {
                console.error('获取 LED 状态失败:', error);
            });
    }

    // 更新 LED 状态
    function updateLED(state, brightness) {
        const formData = new FormData();
        formData.append('state', state ? '1' : '0');
        formData.append('brightness', brightness.toString());

        fetch('/api/led', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            ledStatus.textContent = data.state ? '开启' : '关闭';
        })
        .catch(error => {
            console.error('更新 LED 状态失败:', error);
        });
    }

    // 获取设备配置
    function fetchDeviceConfig() {
        fetch('/api/config')
            .then(response => response.json())
            .then(data => {
                // 更新设备信息
                if (ipAddressElement) ipAddressElement.textContent = data.current_ip;
                if (hostnameDisplayElement) hostnameDisplayElement.textContent = data.hostname || '-';

                // 填充表单字段
                document.getElementById('hostname').value = data.hostname || '';
                document.getElementById('wifiSSID').value = data.wifi_ssid || '';
                document.getElementById('wifiPassword').value = data.wifi_password || '';

                // 设置电机速度滑块
                if (motorSpeedSlider && data.motor_speed !== undefined) {
                    motorSpeedSlider.value = data.motor_speed;
                    if (motorSpeedValue) {
                        motorSpeedValue.textContent = data.motor_speed;
                    }
                }

                // 如果有静态 IP 设置字段，填充它们
                const staticIPField = document.getElementById('staticIP');
                const gatewayField = document.getElementById('gateway');
                const subnetField = document.getElementById('subnet');

                if (staticIPField) staticIPField.value = data.static_ip || '';
                if (gatewayField) gatewayField.value = data.gateway || '';
                if (subnetField) subnetField.value = data.subnet || '';

                // 更新其他设备信息字段
                const macAddressElement = document.getElementById('macAddress');
                const rssiElement = document.getElementById('rssi');
                const firmwareVersionElement = document.getElementById('firmwareVersion');

                if (macAddressElement) macAddressElement.textContent = data.mac_address || '';
                if (rssiElement) rssiElement.textContent = data.rssi ? data.rssi + ' dBm' : '';
                if (firmwareVersionElement) firmwareVersionElement.textContent = data.firmware_version || '';
            })
            .catch(error => {
                console.error('获取设备配置失败:', error);
            });
    }

    // 重启设备
    function restartDevice() {
        fetch('/api/restart', {
            method: 'POST'
        })
        .then(response => response.json())
        .then(data => {
            alert(data.message);
            // 显示重启中的提示
            document.body.innerHTML = '<div style="text-align:center;padding:50px;"><h1>设备正在重启</h1><p>请稍等片刻，页面将在 15 秒后自动刷新...</p></div>';
            // 15 秒后刷新页面
            setTimeout(function() {
                window.location.reload();
            }, 15000);
        })
        .catch(error => {
            console.error('重启设备失败:', error);
            alert('重启设备失败，请手动重启。');
        });
    }
});
