FROM python:3.10-slim-bullseye
ENV VERSION 1.0.0

LABEL name="SmartUFC-DeviceConfiguration"
LABEL version=$VERSION
LABEL maintainer="Michel Bonfim <michelsb@ufc.br>"
LABEL org.opencontainers.image.url="https://github.com/michelsb/SmartUFC-DeviceConfiguration"
LABEL org.opencontainers.image.documentation="https://github.com/michelsb/SmartUFC-DeviceConfiguration"
LABEL org.opencontainers.image.source="https://github.com/michelsb/SmartUFC-DeviceConfiguration/docker"
LABEL org.opencontainers.image.version=$VERSION
LABEL org.opencontainers.image.vendor="Michel Bonfim"
LABEL org.opencontainers.image.title="SmartUFC-DeviceConfiguration"
LABEL org.opencontainers.image.description="A web-based system that enables automatic configuration of devices associated to FIWARE-based SmartUFC sites"
LABEL org.opencontainers.image.licenses="GPL-3.0-or-later"

COPY ./ /app
WORKDIR /app

RUN pip install --no-cache-dir -r requirements.txt

CMD [ "python3", "app.py" ]