FROM mcr.microsoft.com/devcontainers/base:1-ubuntu-22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG ORIGINAL_USER=vscode
ARG NEW_USER=openflap

# Install additional packages
RUN apt-get update && apt-get install -y -q \
    wget \
    figlet \
    gdb-multiarch \
    cmake \
    srecord \
    sudo

# Change user name from vscode to openflap
RUN usermod -l ${NEW_USER} ${ORIGINAL_USER} && \
    groupmod -n ${NEW_USER} ${ORIGINAL_USER} && \
    usermod -d /home/${NEW_USER} -m ${NEW_USER} 
    #&& \
    # echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${NEW_USER} && \
    # chmod 0440 /etc/sudoers.d/${NEW_USER}

# Add user to dialout and plugdev group
RUN adduser ${NEW_USER} dialout
RUN adduser ${NEW_USER} plugdev
RUN adduser ${NEW_USER} sudo

# Allow all users in the sudo group to use sudo without a password
RUN echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/nopasswd && \
    chmod 0440 /etc/sudoers.d/nopasswd


# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

WORKDIR /tmp

# Create symlink to new gdb
RUN ln -s /usr/bin/gdb-multiarch /usr/bin/arm-none-eabi-gdb

# install pyocd
RUN apt -y install python3 python3-pip netcat && \
    python3 -mpip install -U pyocd && \
    wget https://www.python.org/ftp/python/3.8.12/Python-3.8.12.tgz && \
    tar xzf Python-3.8.12.tgz && \
    cd Python-3.8.12 && \
    ./configure --enable-optimizations && \
    make altinstall

# install arm toolchain
RUN mkdir -p /opt/gcc-arm/ && \
    wget https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz && \
    tar xvf arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz -C /opt/gcc-arm/ && \
    cd /opt/gcc-arm/ && \
    chown -R root:root arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/

RUN rm -rf /tmp/*

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}
USER ${NEW_USER}

# Install the bashrc addons
COPY .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful


CMD ["/bin/bash", "-c"]