FROM gcc:12

WORKDIR /app/server

# Copia tutti i file
COPY . .

# Compilazione
RUN gcc -o server main.c funzioni.c -Wall -O2 -pthread

EXPOSE 9100

CMD ["./server"]
