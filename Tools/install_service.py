#!/usr/bin/env python3

# MIT License
# Copyright (c) 2025 _VIFEXTech
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import subprocess
import sys
import os
import argparse


def run_command(command):
    try:
        subprocess.run(command, check=True, shell=True)
        print(f"{command} executed successfully!")
    except subprocess.CalledProcessError:
        print(f"{command} failed!")
        sys.exit(1)


def install_service(
    python_bin, service_name, script_path, service_path, params, working_dir
):
    service_content = f"""[Unit]
Description=DutyCycle Script Service
After=network.target

[Service]
Type=simple
ExecStart={python_bin} {script_path} {params}
WorkingDirectory={working_dir}
Restart=on-failure
RestartSec=5
User={os.getlogin()}
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
"""

    with open(service_path, "w") as service_file:
        service_file.write(service_content)

    run_command("systemctl daemon-reload")
    run_command(f"systemctl enable dutycycle.{service_name}.service")
    run_command(f"systemctl start dutycycle.{service_name}.service")
    print("Service installation successful!")


def uninstall_service(service_name, service_path):
    run_command(f"systemctl stop dutycycle.{service_name}.service")
    run_command(f"systemctl disable dutycycle.{service_name}.service")

    try:
        os.remove(service_path)
        print("Service file deleted successfully!")
    except OSError:
        print("Service file deletion failed!")
        sys.exit(1)

    print("Service uninstallation successful!")


def check_service():
    print("Checking all dutycycle services:")
    try:
        result = subprocess.run(
            "systemctl list-units --all --no-legend 'dutycycle.*.service'",
            check=True,
            shell=True,
            capture_output=True,
            text=True,
        )
        services = [line.split()[0] for line in result.stdout.splitlines()]

        if not services:
            print("No dutycycle services found")
            return

        for service in services:
            try:
                subprocess.run(
                    f"systemctl is-active --quiet {service}", check=True, shell=True
                )
                print(f"{service}: running")
            except subprocess.CalledProcessError:
                print(f"{service}: not running")
                print(f"Run 'sudo journalctl -u {service} -f' for logs")

    except subprocess.CalledProcessError as e:
        print("Failed to list services:", e)


def main():
    if os.getuid() != 0:
        print("Please run this script with sudo!")
        sys.exit(1)

    parser = argparse.ArgumentParser(description="DutyCycle service management tool")
    subparsers = parser.add_subparsers(
        dest="command", required=True, help="Available commands"
    )

    # Install command
    install_parser = subparsers.add_parser(
        "install", help="Install the DutyCycle service"
    )
    install_parser.add_argument(
        "params",
        nargs=argparse.REMAINDER,
        help="Configuration parameters to pass to config script",
    )
    install_parser.add_argument(
        "--service-name",
        default="dutycycle",
        required=True,
        help="Custom service name",
    )
    install_parser.add_argument(
        "--config-script",
        required=True,
        help="Custom config script path",
    )

    # Uninstall command
    uninstall_parser = subparsers.add_parser("uninstall", help="Uninstall the service")
    uninstall_parser.add_argument(
        "--service-name",
        required=True,
        help="Custom service name",
    )

    # Check command
    check_parser = subparsers.add_parser(
        "check", help="Check the DutyCycle service status"
    )

    args = parser.parse_args()
    pwd = os.getcwd()

    if args.command == "install":
        python_bin = (
            subprocess.check_output(
                "command -v python3 || echo /usr/bin/python3", shell=True
            )
            .decode()
            .strip()
        )
        script_path = os.path.join(
            os.path.dirname(os.path.abspath(__file__)), args.config_script
        )
        service_path = f"/etc/systemd/system/dutycycle.{args.service_name}.service"
        params = " ".join(args.params) if args.params else ""
        install_service(
            python_bin, args.service_name, script_path, service_path, params, pwd
        )
    elif args.command == "uninstall":
        service_path = f"/etc/systemd/system/dutycycle.{args.service_name}.service"
        uninstall_service(args.service_name, service_path)
    elif args.command == "check":
        check_service()
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
