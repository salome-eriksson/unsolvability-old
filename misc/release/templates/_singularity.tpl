# This file has been automatically generated.

Bootstrap: docker
From: aibasel/downward:$TAG

%setup
    # Just for diagnosis purposes
    hostname -f > $$SINGULARITY_ROOTFS/etc/build_host

%post

%labels
Name        Fast Downward
Description Singularity image for the Fast Downward planning system
