FROM gcc:12

WORKDIR /app/client

COPY . .

RUN gcc -o client main.c funzioni.c -Wall -O2 -pthread

CMD ["./client"]
