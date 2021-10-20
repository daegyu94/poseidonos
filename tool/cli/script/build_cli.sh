#!/bin/bash

GO=/usr/local/go/bin/go

ROOT_DIR=$(readlink -f $(dirname $0))/
cd $ROOT_DIR/../

:<<END
wget -q --tries=1 --timeout=3 --spider http://google.com

if [[ $? -eq 0 ]]; then
	echo "Online"
	go mod vendor	

else
	echo "Offline"
	rm -rf vendor/pnconnector
	cp -rf ../pnconnector ./vendor/
	rm -rf vendor/dagent
	mkdir vendor/dagent
	cp -rf ../dagent/src ./vendor/dagent/
fi
END

if [ -d "../pnconnector" ]; then
	rm -rf vendor/pnconnector
	cp -rf ../pnconnector ./vendor/
	rm -rf vendor/dagent
	mkdir vendor/dagent
	cp -rf ../dagent/src ./vendor/dagent/
fi

export GIT_COMMIT_CLI=$(git rev-list -1 HEAD)
export BUILD_TIME_CLI=$(date +%s)

./vendor/pnconnector/script/build_resource.sh
${GO} build -mod vendor -tags debug,ssloff -ldflags "-X cli/cmd.GitCommit=$GIT_COMMIT_CLI -X cli/cmd.BuildTime=$BUILD_TIME_CLI"

mv ./cli bin/poseidonos-cli
