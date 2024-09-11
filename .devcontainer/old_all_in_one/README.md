# OpenFlap dev container 

## How to use the devcontainer

[tutorial](https://code.visualstudio.com/docs/devcontainers/tutorial)

- Open this directory in vscode.
- Make sure the `Dev Containers` extension is installed.
- Open the command pallet with `F1` of `Ctrl+Shif+P` and select `Dev Containers: Open Workspace In Container`.
- The devcointainer will build.


## Adding USB devices to the devcontainer

Based on [this stack overflow answer](https://stackoverflow.com/a/66427245).

Use the files in `.devcontainer/utils`.

On your host machine:
```bash
sudo cp .devcontainer/utils/99-docker-tty.rules /etc/udev/rules.d/99-docker-tty.rules
sudo service udev restart
sudo cp .devcontainer/utils/docker_tty.sh /usr/local/bin/docker_tty.sh
```

## Using git in your container

Vscode should automatically forward your git & ssh credentials if your ssh-agent is running. Otherwise [check this](https://code.visualstudio.com/remote/advancedcontainers/sharing-git-credentials).
