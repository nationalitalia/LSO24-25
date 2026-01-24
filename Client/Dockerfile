FROM openjdk:17-jdk-slim

WORKDIR /app/client

# Installa le librerie necessarie per le GUI Java su Linux
RUN apt-get update && apt-get install -it \
    libxext6 \
    libxrender1 \
    libxtst6 \
    libxi6 \
    && rm -rf /var/lib/apt/lists/*

# Copia il file sorgente (assumendo si chiami TrisClient.java)
COPY TrisClient.java .

# Compila il file Java
RUN javac TrisClient.java

# Comando per avviare il client
CMD ["java", "TrisClient"]