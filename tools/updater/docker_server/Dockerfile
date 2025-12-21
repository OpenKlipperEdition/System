FROM nginx
COPY ./sources.list /etc/apt/sources.list
RUN mkdir /ota
RUN mkdir /ssl
RUN apt-get update
RUN apt-get install -y net-tools
RUN apt-get install -y procps
RUN apt-get install -y python-flask
