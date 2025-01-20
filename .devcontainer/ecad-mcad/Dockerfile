# FROM ghcr.io/inti-cmnb/kicad8_auto:1.8.0
FROM ghcr.io/inti-cmnb/kicad_auto_full:dev_k8_1.8.3-6506638_k8.0.6_d_sid_b3.5.1

ARG NEW_USER=openflap
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# Install additional packages
RUN apt update && apt install -y \
    wget \
    figlet \
    ssh \
    less \
    curl \
    unzip \
    bash-completion \
    python3 \
    python3-pip \
    llvm \
    build-essential \
    sudo

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && apt install git-lfs

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# Create a non-root user with sudo rights
RUN useradd -ms /bin/bash ${NEW_USER} -u ${USER_UID} -U -G sudo && \
    echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/${NEW_USER} && \
    chmod 0440 /etc/sudoers.d/${NEW_USER}

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}

# Set the locale
ENV LC_ALL=C

# Install the bashrc addons
COPY --chown=${NEW_USER} .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful

# Install RedHatMono font
RUN wget https://github.com/RedHatOfficial/RedHatFont/raw/4.0.3/fonts/mono/RedHatMono.ttf -P ${HOME}/.fonts

###### Install FreeCAD ######

WORKDIR /tmp
RUN wget https://github.com/FreeCAD/FreeCAD/releases/download/1.0rc2/FreeCAD_1.0.0RC2-conda-Linux-x86_64-py311.AppImage
RUN chmod +x FreeCAD*.AppImage && ./FreeCAD*.AppImage --appimage-extract
RUN mv squashfs-root /opt/FreeCAD && rm -rf FreeCAD*.AppImage
RUN pip install colorama --target /opt/FreeCAD/usr/lib/python3.11/site-packages

###### Setup KiCad ######

# Set Kicad ENV vars
ENV KICAD8_SYMBOL_DIR=/usr/share/kicad/symbols
ENV KICAD8_FOOTPRINT_DIR=/usr/share/kicad/footprints
ENV KICAD8_3DMODEL_DIR=/usr/share/kicad/3dmodels
ENV KICAD8_3RD_PARTY=$HOME/.local/share/kicad/8.0/3rdparty

# Install ESP KiCad libraries
WORKDIR /tmp
RUN wget https://github.com/espressif/kicad-libraries/releases/download/3.0.1/espressif-kicad-addon.zip
RUN unzip espressif-kicad-addon.zip
RUN mkdir -p  $KICAD8_3RD_PARTY/footprints/com_github_espressif_kicad-libraries && cp -r footprints/Espressif.pretty $KICAD8_3RD_PARTY/footprints/com_github_espressif_kicad-libraries/
RUN mkdir -p  $KICAD8_3RD_PARTY/symbols/com_github_espressif_kicad-libraries && cp -r symbols/Espressif.kicad_sym $KICAD8_3RD_PARTY/symbols/com_github_espressif_kicad-libraries/
RUN mkdir -p  $KICAD8_3RD_PARTY/3dmodels/com_github_espressif_kicad-libraries && cp -r 3dmodels/espressif.3dshapes $KICAD8_3RD_PARTY/3dmodels/com_github_espressif_kicad-libraries/
RUN chown -R ${NEW_USER}:${NEW_USER} ${HOME}

# Remove broken .wrl files
RUN rm $KICAD8_3RD_PARTY/3dmodels/com_github_espressif_kicad-libraries/espressif.3dshapes/*.wrl
# Add the libraries to the sym-lib-table and fp-lib-table
RUN sed -i '$i \ \ (lib (name "PCM_Espressif")(type "KiCad")(uri ${KICAD8_3RD_PARTY}/symbols/com_github_espressif_kicad-libraries/Espressif.kicad_sym)(options "")(descr "Espressif symbols"))' /usr/share/kicad/template/sym-lib-table
RUN sed -i '$i \ \ (lib (name "PCM_Espressif")(type "KiCad")(uri ${KICAD8_3RD_PARTY}/footprints/com_github_espressif_kicad-libraries/Espressif.pretty)(options "")(descr "Espressif footprints"))' /usr/share/kicad/template/fp-lib-table

###### Install gerbolyze ######

# Add the Google Chrome repository and install Chrome
WORKDIR /tmp
RUN apt update -qq -y && \
    apt install -y \
        libasound2 \
        libatk-bridge2.0-0 \
        libnss3 \
        libcups2 \
        libdrm2 \
        xdg-utils \
        libxkbcommon-x11-0 \
        libxdamage1 \
        libxfixes3 \
        libxrandr2 \
        libgbm1 \
        libpango-1.0-0 \
        libcairo2 && \
    wget -q -O chrome-linux64.zip https://bit.ly/chrome-linux64-121-0-6167-85 && \
    unzip chrome-linux64.zip && \
    rm chrome-linux64.zip && \
    mv chrome-linux64 /opt/chrome/ && \
    ln -s /opt/chrome/chrome /usr/local/bin/ && \
    wget -q -O chromedriver-linux64.zip https://bit.ly/chromedriver-linux64-121-0-6167-85 && \
    unzip -j chromedriver-linux64.zip chromedriver-linux64/chromedriver && \
    rm chromedriver-linux64.zip && \
    mv chromedriver /usr/local/bin/

# install seleniums
RUN apt install -y python3-selenium 

# install gerbolyze
WORKDIR /tmp
RUN git clone --recurse-submodule https://github.com/jaseg/gerbolyze.git && \
    cd gerbolyze && \
    python3 setup.py install && \
    cd svg-flatten && \
    make && \
    make install

# Get Rust
WORKDIR /tmp
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="${HOME}/.cargo/bin:${PATH}"
RUN cargo install usvg

############################################

# Clear /tmp
RUN rm -rf /tmp/*
WORKDIR ${HOME}

# Set user and start bash
USER ${NEW_USER}
CMD ["/bin/bash", "-c"]