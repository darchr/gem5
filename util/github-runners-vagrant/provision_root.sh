#!/usr/bin/env bash

# fail on unset variables and command errors
set -eu -o pipefail # -x: is for debugging

apt-get update
apt-get upgrade -y
apt-get install -y software-properties-common
add-apt-repository --yes --update ppa:git-core/ppa
apt-get install -y \
  bash \
  build-essential \
  clang-format \
  git \
  git-lfs \
  jq \
  libffi-dev \
  libssl-dev \
  nkf \
  python3 \
  python3-dev \
  python3-pip \
  python3-venv \
  shellcheck \
  tree \
  wget \
  yamllint \
  zstd
snap install jq

# Install docker
apt-get install -y apt-transport-https ca-certificates curl gnupg lsb-release
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | gpg --dearmor -o /usr/share/keyrings/docker-archive-keyring.gpg
echo "deb [arch=amd64 signed-by=/usr/share/keyrings/docker-archive-keyring.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | tee /etc/apt/sources.list.d/docker.list > /dev/null
apt-get update -y
apt-get install -y docker-ce docker-ce-cli containerd.io
groupadd docker || true
gpasswd -a vagrant docker
newgrp docker
systemctl restart docker

# Cleanup
apt-get autoremove -y
