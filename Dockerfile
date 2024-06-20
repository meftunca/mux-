# # use latest alpine image
# FROM alpine:latest

# # son nodejs versiyonunu yükle
# RUN apk add --update nodejs npm
# # bazelisk'i kur
# RUN npm install -g @bazel/bazelisk
# RUN node -v && npm -v && ls -l
# RUN apk add --no-cache bash make
# RUN which bazel
# RUN mkdir /workspace
# WORKDIR /workspace
# COPY . /workspace
# # bazelisk'i bazel'e linkle
# USER bazel

# # bazelisk'in çalıştığını kontrol et
# # RUN bazelisk version


# RUN bazel build src:kqueue
# # # Docker dosyasını çalıştır
# # CMD ["make", "expr"]
# Bazel'in resmi Docker imajını kullan
FROM l.gcr.io/google/bazel:latest

# Çalışma dizinini ayarla
WORKDIR /workspace

# Proje bağımlılıklarını kopyala
COPY WORKSPACE /workspace/WORKSPACE
COPY WORKSPACE /workspace/WORKSPACE.bazel

# Projeyi kopyala
COPY . /workspace

# Projeyi build et ve çalıştır
CMD ["bazel", "run", "//src:kqueue"] 