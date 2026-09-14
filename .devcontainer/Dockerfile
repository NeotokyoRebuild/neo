# The CI image (.github/workflows/cibuild.yml) plus a non-root account, which
# the sniper SDK does not ship. Without one, a rootful-docker container writes
# root-owned files into the bind mount. devcontainer.json passes the host
# username, and updateRemoteUserUID rewrites the UID/GID to match at build time.
FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest

ARG USERNAME=dev
ARG USER_UID=1000
ARG USER_GID=1000

RUN groupadd --gid "${USER_GID}" "${USERNAME}" \
    && useradd --uid "${USER_UID}" --gid "${USER_GID}" -m -s /bin/bash "${USERNAME}" \
    && echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/${USERNAME}" \
    && chmod 0440 "/etc/sudoers.d/${USERNAME}"
