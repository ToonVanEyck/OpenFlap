FROM ghcr.io/inti-cmnb/kicad8_auto:1.7.0

ARG NEW_USER=openflap

# Install additional packages
RUN apt-get update && apt-get install -y -q \
    wget \
    figlet \
    ssh \
    sudo

# Create a non-root user with sudo rights
RUN useradd -ms /bin/bash ${NEW_USER} && \
    usermod -aG sudo ${NEW_USER} && \
    echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${NEW_USER} && \
    chmod 0440 /etc/sudoers.d/${NEW_USER}

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}
USER ${NEW_USER}

# Install the bashrc addons
COPY .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful

# Install RedHatMono font
RUN wget https://github.com/RedHatOfficial/RedHatFont/raw/4.0.3/fonts/mono/RedHatMono.ttf -P ${HOME}/.fonts

CMD ["/bin/bash", "-c"]