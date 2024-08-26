git submodule add https://github.com/jupp0r/prometheus-cpp.git
git submodule update --init --recursive

cd prometheus-cpp
	git checkout v1.2.4
cd ..

git commit -m "Add submodule at specific tag <tag-name>"