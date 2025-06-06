cd ../simulation
./ns3 configure
./ns3 build

cd ../utils
python3 -m venv venv
source venv/bin/activate
pip install pandas matplotlib
deactivate
