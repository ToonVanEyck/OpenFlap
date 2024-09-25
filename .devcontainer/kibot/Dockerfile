FROM ghcr.io/inti-cmnb/kicad8_auto:1.8.0

ARG NEW_USER=openflap
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Install additional packages
RUN apt update && apt install -y \
    wget \
    figlet \
    ssh \
    less \
    sudo

# Create a non-root user with sudo rights
RUN useradd -ms /bin/bash ${NEW_USER} -u ${USER_UID} -U -G sudo && \
    echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${NEW_USER} && \
    chmod 0440 /etc/sudoers.d/${NEW_USER}

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}
USER ${NEW_USER}

# Set the locale
ENV LC_ALL=C

# Install the bashrc addons
COPY --chown=${NEW_USER} .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful

# Install RedHatMono font
RUN wget https://github.com/RedHatOfficial/RedHatFont/raw/4.0.3/fonts/mono/RedHatMono.ttf -P ${HOME}/.fonts

CMD ["/bin/bash", "-c"]