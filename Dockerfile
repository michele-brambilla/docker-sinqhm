FROM centos:7 as builder

RUN yum -y install epel-release && \
     yum -y install gcc gcc-c++ git make mongo-c-driver-devel zeromq-devel libbson-devel mxml-devel

COPY sinqhm sinqhm

ENV C_INCLUDE_PATH /usr/include/libbson-1.0:$C_INCLUDE_PATH
ENV C_INCLUDE_PATH /usr/include/libbson-1.0:$C_INCLUDE_PATH

RUN cd sinqhm/develop/apps && \
    tar -xzvf appweb-src-2.4.2.tar.gz && cd appweb-src-2.4.2 && \
    ./configure && make && cp lib/libappwebStatic.a ../../lib/x86_64/

RUN cd sinqhm/develop/src && make


FROM builder

RUN yum -y install epel-release && \
    yum -y install gcc gcc-c++ git make mongo-c-driver zeromq libbson mxml &&\
    yum -y remove epel-release && yum clean all && \
    rm -rf /var/cache/yum && \
    mv sinqhm/develop/src/ua/rita.xml sinqhm/develop/src/ua/sinqhm.xml 
   
COPY --from=builder sinqhm/develop/src/rt sinqhm/rt 
COPY --from=builder sinqhm/develop/src/ua sinqhm/ua 
COPY --from=builder sinqhm/develop/src/ua/rita.xml sinqhm/ua/sinqhm.xml 
COPY run.sh run.sh 

EXPOSE 8080

ENTRYPOINT ["/run.sh"]
CMD ["5777"]
