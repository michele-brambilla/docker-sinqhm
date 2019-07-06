# SINQHM
FROM centos:7 as hm_builder

RUN yum -y install epel-release && \
     yum -y install gcc gcc-c++ git make mongo-c-driver-devel zeromq-devel libbson-devel mxml-devel

COPY sinqhm sinqhm

ENV C_INCLUDE_PATH /usr/include/libbson-1.0:$C_INCLUDE_PATH
ENV C_INCLUDE_PATH /usr/include/libbson-1.0:$C_INCLUDE_PATH

RUN cd sinqhm/develop/apps && \
    tar -xzvf appweb-src-2.4.2.tar.gz && cd appweb-src-2.4.2 && \
    ./configure && make && cp lib/libappwebStatic.a ../../lib/x86_64/

RUN cd sinqhm/develop/src && make


# zmqGenerator
FROM centos:7 as gen_builder

RUN yum install -y epel-release && \
    yum install -y git gcc zeromq-devel hdf5-devel make &&\
    git clone --single-branch --branch use_hdf5 https://github.com/michele-brambilla/neventGenerator.git &&\
    cd neventGenerator && make



FROM centos:7

RUN yum -y install epel-release && \
    yum -y install glibc mongo-c-driver-libs &&\
    yum -y remove epel-release && yum clean all && \
    rm -rf /var/cache/yum && touch /tmp/lwlfifo

COPY --from=hm_builder sinqhm/develop/src/rt/sinqhm_filler sinqhm/rt/sinqhm_filler
COPY --from=hm_builder sinqhm/develop/src/ua/ sinqhm/ua/
COPY --from=hm_builder sinqhm/develop/src/ua/rita.xml sinqhm/ua/sinqhm.xml
COPY --from=hm_builder /lib64/libbson-1.0.so.0 /lib64/libbson-1.0.so.0
COPY --from=hm_builder /lib64/libbson-1.0.so.0.0.0 /lib64/libbson-1.0.so.0.0.0
COPY --from=hm_builder /lib64/libmxml.so.1 /lib64/libmxml.so.1
COPY --from=hm_builder /lib64/libmxml.so.1.6 /lib64/libmxml.so.1.6
RUN rm sinqhm/ua/*.o

COPY run.sh run.sh


COPY --from=gen_builder /lib64/libzmq.so /lib64/
COPY --from=gen_builder /lib64/libhdf5.so.8 /lib64/libhdf5.so.8
COPY --from=gen_builder /lib64/libhdf5.so.8.0.1 /lib64/libhdf5.so.8.0.1
COPY --from=gen_builder /lib64/libzmq.so.5 /lib64/libzmq.so.5
COPY --from=gen_builder /lib64/libzmq.so.5.0.0 /lib64/libzmq.so.5.0.0
COPY --from=gen_builder /lib64/libsodium.so.23 /lib64/libsodium.so.23
COPY --from=gen_builder /lib64/libsodium.so.23.3.0 /lib64/libsodium.so.23.3.0
COPY --from=gen_builder /lib64/libhdf5_hl.so.8 /lib64/libhdf5_hl.so.8
COPY --from=gen_builder /lib64/libhdf5_hl.so.8.0.1 /lib64/libhdf5_hl.so.8.0.1
COPY --from=gen_builder /lib64/libsz.so.2 /lib64/libsz.so.2
COPY --from=gen_builder /lib64/libsz.so.2.0.1 /lib64/libsz.so.2.0.1
COPY --from=gen_builder /lib64/libpgm-5.2.so.0 /lib64/libpgm-5.2.so.0
COPY --from=gen_builder /lib64/libpgm-5.2.so.0.0.122 /lib64/libpgm-5.2.so.0.0.122
COPY --from=gen_builder /lib64/libaec.so.0 /lib64/libaec.so.0
COPY --from=gen_builder /lib64/libaec.so.0.0.10 /lib64/libaec.so.0.0.10

COPY --from=gen_builder neventGenerator/zmqGenerator neventGenerator/zmqGenerator
COPY --from=gen_builder neventGenerator/zmqReader neventGenerator/zmqReader
COPY --from=gen_builder neventGenerator/rita22012n006190.hdf neventGenerator/rita22012n006190.hdf


EXPOSE 8080

ENTRYPOINT ["/run.sh"]
CMD ["5777"]
