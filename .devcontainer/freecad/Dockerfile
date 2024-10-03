FROM mcr.microsoft.com/devcontainers/base:1-ubuntu-22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG ORIGINAL_USER=vscode
ARG NEW_USER=openflap

# Install additional packages
# Install additional packages
RUN apt-get update && apt-get install -y -q \
    wget \
    curl \
    figlet \
    python3 \
    python3-pip \
    sudo

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# Change user name from vscode to openflap
RUN usermod -l ${NEW_USER} ${ORIGINAL_USER} && \
    groupmod -n ${NEW_USER} ${ORIGINAL_USER} && \
    usermod -d /home/${NEW_USER} -m ${NEW_USER} && \
    adduser ${NEW_USER} sudo && \
    echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/nopasswd && \
    chmod 0440 /etc/sudoers.d/nopasswd

# Install FreeCAD

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}
USER ${NEW_USER}

# Install the bashrc addons
COPY --chown=${NEW_USER} .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful

CMD ["/bin/bash", "-c"]