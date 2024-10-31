FROM espressif/idf:v5.3.1

ARG DEBIAN_FRONTEND=nointeractive
ARG NEW_USER=openflap
ARG USER_UID=1000

# Install additional packages
RUN apt update && apt install -y \
    libusb-1.0-0-dev \
    ruby-full \
    figlet \
    libdbus-1-dev \
    libbsd-dev \
    libglib2.0-0 \
    libsdl2-dev \
    libslirp-dev \
    libnuma1 \
    udev \
    libpixman-1-0 \
    cmake \
    git-core \
    bash-completion \
    libfuse-dev \
    sudo

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && apt install git-lfs

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# QEMU
ENV QEMU_REL=esp_develop_8.2.0_20240122
ENV QEMU_SHA256=e7c72ef5705ad1444d391711088c8717fc89f42e9bf6d1487f9c2a326b8cfa83
ENV QEMU_DIST=qemu-xtensa-softmmu-${QEMU_REL}-x86_64-linux-gnu.tar.xz
ENV QEMU_URL=https://github.com/espressif/qemu/releases/download/esp-develop-8.2.0-20240122/${QEMU_DIST}

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN wget --no-verbose ${QEMU_URL} && \
    echo "${QEMU_SHA256} *${QEMU_DIST}" | sha256sum --check --strict - && \
    tar -xf $QEMU_DIST -C /opt && \
    rm ${QEMU_DIST}

ENV PATH=/opt/qemu/bin:${PATH}

# Create openflap user 
RUN groupadd --gid ${USER_UID} ${NEW_USER} && \
    adduser --uid ${USER_UID} --gid ${USER_UID} --disabled-password --gecos "" ${NEW_USER} && \
    usermod -a -G dialout,plugdev,sudo ${NEW_USER} && \
    echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${NEW_USER} && \
    chmod 0440 /etc/sudoers.d/${NEW_USER}

# Install ESP-IDF packages
RUN $IDF_PATH/install.sh --enable-ci --enable-pytest
RUN wget https://raw.githubusercontent.com/espressif/esp-idf/master/tools/idf_py_actions/qemu_ext.py -P $IDF_PATH/tools/idf_py_actions/
RUN python $IDF_PATH/tools/idf_tools.py install qemu-xtensa

# Make /opt/esp/ writable
RUN chown -R ${NEW_USER}:${NEW_USER} /opt/esp/

# Install additional packages
RUN apt-get update && apt-get install -y -q \
    netcat-traditional \
    figlet \
    sudo

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}
USER ${NEW_USER}

# Install the bashrc addons
COPY .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful

ENTRYPOINT [ "/opt/esp/entrypoint.sh" ]
CMD ["/bin/bash", "-c"]