FROM eabatalov/aucont-test-base
USER 0
RUN apt-get -y update &&\
	apt-get install -y libboost-program-options-dev iptables
RUN mkdir -p /aucont/fs /aucont/cgroup /run/netns
USER 1000
