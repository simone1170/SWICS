cd ../IPAL/docker
for dir in evaluate ids transcriber
do
    cd "./$dir"
    python3 -m venv venv
    source venv/bin/activate
    pip install -r requirements.txt
    deactivate
    cd ..
done

cd ../../simulation
./ns3 configure
./ns3 build

cd ../utils
python3 -m venv venv
source venv/bin/activate
pip install pandas matplotlib
deactivate