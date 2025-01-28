# Running devcontainers without vscode

Install the devcontainer CLI from [here](https://github.com/devcontainers/cli).

From the top-level directory, run this to build/run the image:

```shell
devcontainer up --workspace-folder . --config .devcontainer/<container dir>/devcontainer.json
```

For example, to start the ecad-mcad container:

```shell
devcontainer up --workspace-folder . --config .devcontainer/ecad-mcad/devcontainer.json
```

To jump into the container, you can run:

```shell
docker exec -it openflap_ecad_mcad /bin/bash
```
