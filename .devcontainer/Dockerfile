FROM mcr.microsoft.com/devcontainers/base:1-ubuntu-22.04

ARG DEBIAN_FRONTEND=noninteractive

RUN : \
  && apt-get update \
  && apt-get install -y \
    apt-utils \
    bison \
    ca-certificates \
    ccache \
    check \
    curl \
    flex \
    git \
    git-lfs \
    gperf \
    lcov \
    libbsd-dev \
    libffi-dev \
    libncurses-dev \
    libusb-1.0-0-dev \
    make \
    ninja-build \
    python3 \
    python3-venv \
    ruby \
    unzip \
    wget \
    xz-utils \
    zip \
    figlet \
    netcat \
    cmake \
  && apt-get autoremove -y \
  && rm -rf /var/lib/apt/lists/* \
  && update-alternatives --install /usr/bin/python python /usr/bin/python3 10 \
  && :

# To build the image for a branch or a tag of IDF, pass --build-arg IDF_CLONE_BRANCH_OR_TAG=name.
# To build the image with a specific commit ID of IDF, pass --build-arg IDF_CHECKOUT_REF=commit-id.
# It is possibe to combine both, e.g.:
#   IDF_CLONE_BRANCH_OR_TAG=release/vX.Y
#   IDF_CHECKOUT_REF=<some commit on release/vX.Y branch>.
# Use IDF_CLONE_SHALLOW=1 to peform shallow clone (i.e. --depth=1 --shallow-submodules)
# Use IDF_INSTALL_TARGETS to install tools only for selected chip targets (CSV)

ARG IDF_CLONE_URL=https://github.com/espressif/esp-idf.git
ARG IDF_CLONE_BRANCH_OR_TAG=release/v5.1
ARG IDF_CHECKOUT_REF=
ARG IDF_CLONE_SHALLOW=1
ARG IDF_INSTALL_TARGETS=all

ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp

# install build essential needed for linux target apps, which is a preview target so it is installed with "all" only
RUN if [ "$IDF_INSTALL_TARGETS" = "all" ]; then \
    apt-get update \
    && apt-get install -y build-essential \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/* ; \
  fi

RUN echo IDF_CHECKOUT_REF=$IDF_CHECKOUT_REF IDF_CLONE_BRANCH_OR_TAG=$IDF_CLONE_BRANCH_OR_TAG && \
    git clone --recursive \
      ${IDF_CLONE_SHALLOW:+--depth=1 --shallow-submodules} \
      ${IDF_CLONE_BRANCH_OR_TAG:+-b $IDF_CLONE_BRANCH_OR_TAG} \
      $IDF_CLONE_URL $IDF_PATH && \
    if [ -n "$IDF_CHECKOUT_REF" ]; then \
      cd $IDF_PATH && \
      if [ -n "$IDF_CLONE_SHALLOW" ]; then \
        git fetch origin --depth=1 --recurse-submodules ${IDF_CHECKOUT_REF}; \
      fi && \
      git checkout $IDF_CHECKOUT_REF && \
      git submodule update --init --recursive; \
    fi

# Install all the required tools
RUN : \
  && update-ca-certificates --fresh \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install required --targets=${IDF_INSTALL_TARGETS} \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install cmake \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install-python-env \
  && rm -rf $IDF_TOOLS_PATH/dist \
  && :

# The constraint file has been downloaded and the right Python package versions installed. No need to check and
# download this at every invocation of the container.
ENV IDF_PYTHON_CHECK_CONSTRAINTS=no

# Ccache is installed, enable it by default
ENV IDF_CCACHE_ENABLE=1

# Install QEMU runtime dependencies
RUN : \
  && apt-get update && apt-get install -y -q \
    libglib2.0-0 \
    libpixman-1-0 \
  && rm -rf /var/lib/apt/lists/* \
  && :

# Install QEMU
ARG QEMU_VER=esp-develop-20220919
ARG QEMU_DIST=qemu-${QEMU_VER}.tar.bz2
ARG QEMU_SHA256=f6565d3f0d1e463a63a7f81aec94cce62df662bd42fc7606de4b4418ed55f870
RUN : \
  && wget --no-verbose https://github.com/espressif/qemu/releases/download/${QEMU_VER}/${QEMU_DIST} \
  && echo "${QEMU_SHA256} *${QEMU_DIST}" | sha256sum --check --strict - \
  && tar -xf ${QEMU_DIST} -C /opt \
  && rm ${QEMU_DIST} \
  && :
ENV PATH=/opt/qemu/bin:${PATH}

RUN echo "source /opt/esp/idf/export.sh" >> /etc/bash.bashrc

RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf && \
    echo "figlet -f 3d.flf Open Flap" >> /etc/bash.bashrc

# install microchip XC8
# RUN wget https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc8-v2.41-full-install-linux-x64-installer.run && \
#     chmod +x xc8-v2.41-full-install-linux-x64-installer.run && \
#     ./xc8-v2.41-full-install-linux-x64-installer.run --mode unattended --netservername localhost --LicenseType FreeMode && \
#     rm xc8*

WORKDIR /tmp

# install all required packages
RUN apt update && export DEBIAN_FRONTEND=noninteractive && \
    apt -y install --no-install-recommends wget figlet clang-format cppcheck hwdata sudo bash-completion ssh gdb-multiarch udev usbutils

# Create symlink to new gdb
RUN ln -s /usr/bin/gdb-multiarch /usr/bin/arm-none-eabi-gdb

# Add user to dialout group to enable rootless tty acces for serial monitor eextention
RUN adduser vscode dialout
RUN adduser vscode plugdev

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

# install ncurses dependency
RUN apt -y install libncurses5 && ln -s /usr/lib/x86_64-linux-gnu/libncursesw.so.6 /usr/lib/x86_64-linux-gnu/libncursesw.so.5

# add a git lgb alias
RUN git config --global alias.lgb "log --graph --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr) %C(bold blue)<%an>%Creset%n' --abbrev-commit --date=relative --branches"

# cleanup tmp
RUN rm -rf /tmp/*
# pyocd gdbserver -u remote:172.20.206.105:5555 --config Misc/pyocd.yaml --target py32f003x8 
# https://stackoverflow.com/questions/42938992/usbip-in-docker