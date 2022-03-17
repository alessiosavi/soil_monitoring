FROM golang:alpine


# The latest alpine images don't have some tools like (`git` and `bash`).
# Adding git, bash and openssh to the image
#RUN apt update && apt upgrade

LABEL maintainer="Alessio Savi <alessiosavibtc@gmail.com>"

# Set the Current Working Directory inside the container
WORKDIR /app

# Copy the source from the current directory to the Working Directory inside the container
COPY . /app
# Download dependencies
RUN go get -v -u
RUN go mod tidy -compat=1.17
RUN go clean

RUN rm /app/soil_monitor.ino

# Build the Go app
RUN go build -o soil_mon

# Expose port 8080 to the outside world
EXPOSE 8080

# Run the executable
CMD ["./soil_mon"]