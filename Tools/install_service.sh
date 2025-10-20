#!/bin/bash

# Check if the script is run with sudo privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "Please run this script with sudo!"
    exit 1
fi

show_help() {
    echo "Usage: $0 <command> <params>"
    echo "Commands:"
    echo "  install     Install the DutyCycle service"
    echo "  uninstall   Uninstall the DutyCycle service"
    echo "  check       Check the DutyCycle service status"
    echo "Params:"
    echo "  Configuration parameters to pass to config_clock.py"
}

if [ $# -lt 1 ]; then
    show_help
    exit 1
fi

cmd=$1
shift
params="$@"

pwd=$(pwd)

echo "Command: $cmd"
echo "Parameters: $params"

if [ "$cmd" == "install" ]; then
    PYTHON_BIN="$(command -v python3 || echo /usr/bin/python3)"
    SCRIPT_PATH="$(pwd)/Tools/config_clock.py"
    SERVICE_PATH=/etc/systemd/system/dutycycle.service
    
    # Create systemd unit with absolute paths and safer settings
    cat > "$SERVICE_PATH" <<EOF
[Unit]
Description=DutyCycle Script Service
After=network.target

[Service]
Type=simple
ExecStart=${PYTHON_BIN} ${SCRIPT_PATH} ${params}
WorkingDirectory=${pwd}
Restart=on-failure
RestartSec=5
User=$(whoami)
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF
    
    # Reload systemd config and enable/start (unchanged)
    if systemctl daemon-reload; then
        echo "systemctl daemon-reload executed successfully!"
    else
        echo "systemctl daemon-reload failed!"
        exit 1
    fi
    
    if systemctl enable dutycycle.service; then
        echo "systemctl enable dutycycle.service executed successfully!"
    else
        echo "systemctl enable dutycycle.service failed!"
        exit 1
    fi
    
    if systemctl start dutycycle.service; then
        echo "systemctl start dutycycle.service executed successfully!"
    else
        echo "systemctl start dutycycle.service failed!"
        exit 1
    fi
    
    echo "Service installation successful!"
    
    elif [ "$cmd" == "uninstall" ]; then
    # Stop service
    if systemctl stop dutycycle.service; then
        echo "systemctl stop dutycycle.service executed successfully!"
    else
        echo "systemctl stop dutycycle.service failed!"
    fi
    
    # Disable service
    if systemctl disable dutycycle.service; then
        echo "systemctl disable dutycycle.service executed successfully!"
    else
        echo "systemctl disable dutycycle.service failed!"
    fi
    
    # Delete service file
    if rm /etc/systemd/system/dutycycle.service; then
        echo "Service file deleted successfully!"
    else
        echo "Service file deletion failed!"
        exit 1
    fi
    
    echo "Service uninstallation successful!"
    
    elif [ "$cmd" == "check" ]; then
    if systemctl is-active --quiet dutycycle.service; then
        echo "Service is running!"
    else
        echo "Service is not running!"
        echo "Please run 'sudo journalctl -u dutycycle.service -f' to check the logs for more information."
        echo "If the 'ModuleNotFoundError' error message is printed, try 'sudo pip3 install -r requirements.txt'"
    fi
else
    echo "Unknown command: $cmd"
    show_help
    exit 1
fi
