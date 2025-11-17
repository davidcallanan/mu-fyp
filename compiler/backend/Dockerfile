FROM ubuntu:24.04

RUN apt-get update

RUN apt-get install -y \
	make \
	# LLVM toolchain
	llvm-17 \
	llvm-17-dev \
	clang-17 \
	lld-17 \
	# One can add cross-compilation dependencies here in future
	&& rm -rf /var/lib/apt/lists/*

RUN ln -sf /usr/bin/clang-17 /usr/bin/clang && \
	ln -sf /usr/bin/clang++-17 /usr/bin/clang++

ENV LLVM_CONFIG=llvm-config-17

WORKDIR /app

COPY . .

RUN mkdir -p /app/out /volume/out

RUN make -j$(nproc) all

RUN chmod +x live.sh

VOLUME ["/volume/out"]
VOLUME ["/volume/in"]

# Final steps that are dynamic (using live volumes) are delegated to this dedicated shell script.
CMD ["bash", "./live.sh"]
