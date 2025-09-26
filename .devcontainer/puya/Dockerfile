FROM mcr.microsoft.com/devcontainers/base:1-ubuntu-22.04

ARG DEBIAN_FRONTEND=noninteractive
ARG ORIGINAL_USER=vscode
ARG NEW_USER=openflap

# Install additional packages
RUN apt update && apt install -y \
    wget \
    figlet \
    gdb-multiarch \
    cmake \
    srecord \
    sudo

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && apt install git-lfs

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# Install Unity testing framework v2.6.1
RUN cd /opt && \
    git clone --depth 1 --branch v2.6.1 https://github.com/ThrowTheSwitch/Unity.git unity && \
    chown -R root:root unity/

# Change user name from vscode to openflap
RUN usermod -l ${NEW_USER} ${ORIGINAL_USER} && \
    groupmod -n ${NEW_USER} ${ORIGINAL_USER} && \
    usermod -d /home/${NEW_USER} -m ${NEW_USER}

# Add user to dialout and plugdev group
RUN adduser ${NEW_USER} dialout
RUN adduser ${NEW_USER} plugdev

# Allow all users in the sudo group to use sudo without a password
RUN adduser ${NEW_USER} sudo
RUN echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/nopasswd && \
    chmod 0440 /etc/sudoers.d/nopasswd

WORKDIR /tmp

# Create symlink to new gdb
RUN ln -s /usr/bin/gdb-multiarch /usr/bin/arm-none-eabi-gdb

# Install lib ncurses5
RUN apt install -y libncurses5
RUN ln -s libncursesw.so.6.3 /usr/lib/x86_64-linux-gnu/libncursesw.so.5

# install pyocd
RUN apt -y install python3 python3-pip netcat && \
    python3 -mpip install -U pyocd && \
    wget https://www.python.org/ftp/python/3.8.12/Python-3.8.12.tgz && \
    tar xzf Python-3.8.12.tgz && \
    cd Python-3.8.12 && \
    ./configure --enable-optimizations && \
    make altinstall

# install some packages for the ota tool.
RUN pip install requests requests_toolbelt tqdm tabulate

# install arm toolchain
RUN mkdir -p /opt/gcc-arm/ && \
    wget https://developer.arm.com/-/media/Files/downloads/gnu/12.3.rel1/binrel/arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz && \
    tar xvf arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi.tar.xz -C /opt/gcc-arm/ && \
    cd /opt/gcc-arm/ && \
    chown -R root:root arm-gnu-toolchain-12.3.rel1-x86_64-arm-none-eabi/

# Install clang-format-18
WORKDIR /tmp
RUN apt update && apt install -y lsb-release software-properties-common gnupg 
RUN wget https://apt.llvm.org/llvm.sh
RUN chmod +x llvm.sh
RUN ./llvm.sh 18 clang-format
RUN apt install -y clang-format-18
RUN ln -s /usr/bin/clang-format-18 /usr/bin/clang-format
RUN rm llvm.sh

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