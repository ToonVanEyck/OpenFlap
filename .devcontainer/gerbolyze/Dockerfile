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

# Install git-lfs
RUN curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.deb.sh | bash && apt install git-lfs

# Install figlet font
RUN cd /usr/share/figlet && wget https://raw.githubusercontent.com/xero/figlet-fonts/master/3d.flf

# Change user name from vscode to openflap
RUN usermod -l ${NEW_USER} ${ORIGINAL_USER} && \
    groupmod -n ${NEW_USER} ${ORIGINAL_USER} && \
    usermod -d /home/${NEW_USER} -m ${NEW_USER} && \
    adduser ${NEW_USER} sudo && \
    echo "%sudo ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/nopasswd && \
    chmod 0440 /etc/sudoers.d/nopasswd

# Add the Google Chrome repository and install Chrome
RUN cd /tmp && \ 
    apt update -qq -y && \
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
RUN pip install selenium 

# install gerbolyze
RUN cd /tmp && \
    git clone --recurse-submodule https://github.com/jaseg/gerbolyze.git && \
    cd gerbolyze && \
    python3 setup.py install && \
    cd svg-flatten && \
    make && \
    make install

# Setup user and home
ENV USER=${NEW_USER}
ENV HOME=/home/${NEW_USER}
WORKDIR ${HOME}
USER ${NEW_USER}

# Install the bashrc addons
COPY --chown=${NEW_USER} .bashrc_devcontainer_addons ${HOME}/.bashrc_devcontainer_addons

# Don't display message about how to use sudo...
RUN touch ${HOME}/.sudo_as_admin_successful

# Get Rust
RUN curl https://sh.rustup.rs -sSf | bash -s -- -y
ENV PATH="${HOME}/.cargo/bin:${PATH}"
RUN cargo install usvg

CMD ["/bin/bash", "-c"]