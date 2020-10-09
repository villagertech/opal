FROM centos:7
RUN yum groupinstall -y "Development tools" && yum clean all
RUN yum install -y rpmdevtools yum-utils && yum clean all
# Fix CentOS7 dist tag macro (defaults to .el7.centos)
RUN sed -i s/\.centos// /etc/rpm/macros.dist
RUN yum install -y epel-release && yum clean all
COPY build/mcu-el7.repo /etc/yum.repos.d/

ARG USERID=1000
ARG GROUPID=1000
RUN groupadd -g ${USERID} rpmbuild && useradd -u ${USERID} -g ${GROUPID} rpmbuild
WORKDIR /home/rpmbuild

# Install devtoolset-9
RUN yum install -y centos-release-scl-rh && \
    yum install -y --setopt=tsflags=nodocs devtoolset-9-gcc devtoolset-9-gcc-c++ devtoolset-9-gcc-gfortran devtoolset-9-gdb && \
    yum clean all

# Install OpenH264
ADD ../plugins/install_openh264.sh /tmp/
RUN /tmp/install_openh264.sh /usr/local/lib64

# Uncomment if you want to install some local dependencies instead of using Nexus yum repos
#COPY build-deps /tmp/build-deps/
#RUN yum install -y /tmp/build-deps/*.rpm && yum clean all

# Install dependencies referenced by the spec file, but cache the installed RPMs so they can be fingerprinted later
ARG SPECFILE
COPY ${SPECFILE} .
# Invalidate Docker cache if yum repo metadata has changed
ADD http://nexus.bbcollab.net/yum/el7/mcu-develop/repodata/repomd.xml /tmp/
RUN yum-builddep -y --downloadonly --downloaddir=/tmp/build-deps ${SPECFILE} && \
    ([ -z "$(ls -A /tmp/build-deps/*.rpm)" ] || yum install -y /tmp/build-deps/*.rpm) && \
    yum clean all

USER rpmbuild
