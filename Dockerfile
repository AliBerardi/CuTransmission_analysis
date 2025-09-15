FROM rootproject/root:latest

ENV DEBIAN_FRONTEND=noninteractive \
    PYTHONUNBUFFERED=1 \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

# System deps (no pip/setuptools from apt)
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        python3-venv \
        python3-dev \
        build-essential \
        libxrootd-dev \
        xrootd-client \
    && rm -rf /var/lib/apt/lists/*

# Create and use a venv
ENV VENV=/opt/venv
RUN python3 -m venv "$VENV"
ENV PATH="$VENV/bin:$PATH"

# Python deps inside venv
RUN python -m pip install --upgrade pip setuptools wheel && \
    pip install --no-cache-dir numpy pandas uproot pybind11

WORKDIR /CuTransmission_analysis
COPY . .
RUN python setup.py build_ext --inplace
RUN chmod +x run_codes.sh

ENTRYPOINT ["./run_codes.sh"]
CMD ["--all"] 