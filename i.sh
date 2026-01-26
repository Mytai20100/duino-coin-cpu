if [ -f /etc/debian_version ]; then
    sudo apt-get update
    sudo apt-get install -y build-essential cmake libssl-dev libcurl4-openssl-dev libjsoncpp-dev libyaml-cpp-dev
elif [ -f /etc/redhat-release ]; then
    sudo yum install -y gcc gcc-c++ make cmake openssl-devel libcurl-devel yaml-cpp-devel
elif [ -f /etc/arch-release ]; then
    sudo pacman -S --needed base-devel cmake openssl curl yaml-cpp
else
    exit 1
fi