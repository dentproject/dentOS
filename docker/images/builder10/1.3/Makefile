VERSION=1.3
USER=dentproject
REPO=builder10

TOOLS=../../../tools/docker_shell ../../../tools/container-id

build: check_version
	cp $(TOOLS) .
	docker build -t $(USER)/$(REPO):$(VERSION) .
	rm -rf $(notdir $(TOOLS))
push:
	docker push $(USER)/$(REPO):$(VERSION)
