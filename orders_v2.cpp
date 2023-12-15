#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <queue>

using namespace std;

class Order {
private:
    static int counter;  // Static member variable to keep track of the count 
public:
    string Order_ID;
    string Client_ID; // Attribute: Client_ID (string of 7 chars)
    string Instrument; // Attribute: Instrument (String out of given 5 options)
    int side; // Attribute: side (int)
    int quantity; // Attribute: quantity (int)
    double price; // Attribute: price (double)
    
    Order(string Client_ID, string Instrument, int side, double price, int quantity) {
        this->Order_ID = "ord"+ to_string(++counter);
        this->Client_ID = Client_ID;
        this->Instrument = Instrument;
        this->side = side;
        this->quantity = quantity;
        this->price = price;
    }
};

int Order::counter=0;

class Exec_report {
public:
    string Order_ID; // Attribute: Order_ID (string of 7 chars)
    string Client_Order_ID; // Attribute: Client_Order_ID (string of 7 chars)
    string Instrument; // Attribute: Instrument (String out of given 5 options)
    int side; // Attribute: side (int)
    string Exec_Status; // Attribute: Exec_Status (String out of given 3 options)
    int quantity; // Attribute: quantity (int)
    double price; // Attribute: price (double)
    string Reason; // Attribute: Reason
    string Time; // Attribute: Time (string of 8 chars)
    
    Exec_report(string Order_ID, string Client_Order_ID, string Instrument, int side, string Exec_Status, int quantity, double price,string Reason, string Time) {
        this->Order_ID = Order_ID;
        this->Client_Order_ID = Client_Order_ID;
        this->Instrument = Instrument;
        this->side = side;
        this->Exec_Status = Exec_Status;
        this->quantity = quantity;
        this->price = price;
        this->Reason = Reason;
        this->Time = Time;
    }
};

// Function to read the CSV file and return a vector of orders
vector<Order> readCSVFile(string filename) {
    
    vector<Order> orders; // Vector of orders ... data not validated

    ifstream file(filename);
    if (! file.is_open()) {
        cout << "Failed to open file." << endl;
        return orders;
    }
    // if the file is readable, read the data
    string line;
    // Skip the header line
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string clientId, instrument;
        int side, quantity;
        double price;

        getline(ss, clientId, ',');
        getline(ss, instrument, ',');
        ss >> side;
        ss.ignore();
        ss >> quantity;
        ss.ignore();
        ss >> price;

        //removed the extra copy created
        orders.emplace_back(clientId, instrument, side, price, quantity);
    }
    file.close();
    return orders;
}

void writeToCSVFile(string filename, const vector<string>& attributes, const vector<Exec_report>& data) {
    ofstream file(filename, ios::out); // Open the file in write mode
    if (!file.is_open()) {
        cout << "Failed to open file." << endl;
        return;
    }

    // Write attribute names
    for (size_t i = 0; i < attributes.size(); i++) {
        file << attributes[i];
        if (i != attributes.size() - 1) {
            file << ",";
        }
    }
    file << endl;

    // Write data to the file
    for (const auto& report : data) {
        file << report.Order_ID << "," << report.Client_Order_ID << "," << report.Instrument << "," << report.side << "," << report.Exec_Status << "," << report.quantity << "," << report.price << "," << report.Reason <<  "," << report.Time << endl;
    }

    file.close();
}

string getCurrentTimestamp() {
    // Get the current time point
    auto now = chrono::system_clock::now();

    // Convert the time point to a time_t object
    time_t time = chrono::system_clock::to_time_t(now);

    // Convert time_t to a struct tm in UTC
    tm* tm_utc = gmtime(&time);

    // Create a time string with the desired format
    ostringstream oss;
    oss << put_time(tm_utc, "%Y%m%d-%H%M%S");
    
    // Get milliseconds and append to the timestamp
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    oss << '.' << setfill('0') << setw(3) << milliseconds.count();

    return oss.str();
}



// Comparator for max heap (buying orders)
struct MaxHeapComparator {
    bool operator()(const Order* a, const Order* b) const {
        if (a->price == b->price) {
            return a->Order_ID > b->Order_ID; // Prioritize by arrival order
        }
        return a->price < b->price;
    }
};

// Comparator for min heap (selling orders)
struct MinHeapComparator {
    bool operator()(const Order* a, const Order* b) const {
        if (a->price == b->price) {
            return a->Order_ID > b->Order_ID; // Prioritize by arrival order
        }
        return a->price > b->price;
    }
};

// data validation
string validateData(Order order){
    string ret="";
    if (order.Client_ID.length() > 7 || order.Client_ID==""){
        ret+= "Invalid Client Order ID. ";
    }
    if (order.Instrument != "Rose" && order.Instrument != "Lavender" && order.Instrument != "Lotus" && order.Instrument != "Tulip" && order.Instrument != "Orchid"){
        ret+= "Invalid instrument. ";
    }
    if (order.side != 1 && order.side != 2){
        ret+= "Invalid side. ";
    }
    if (order.price <= 0){
        ret+= "Invalid price. ";
    }
    
    if (order.quantity < 10 || order.quantity > 1000 || order.quantity % 10 != 0){
        ret+= "Invalid size. ";
    }
    return ret;
}

void handleOrders (vector<Order> &orders){
    priority_queue<Order*, vector<Order*>, MaxHeapComparator> buyHeap; // Max heap for buying orders
    priority_queue<Order*, vector<Order*>, MinHeapComparator> sellHeap; // Min heap for selling orders

    vector<Exec_report> data;

    // Separate orders into buy and sell heaps
    for (int i=0; i<orders.size(); i++) {
        bool partially_filled = false;
        // data validation
        string ret = validateData(orders[i]);
        cout << ret << endl;
        if (ret !=""){
            data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Reject", orders[i].quantity, orders[i].price,ret, getCurrentTimestamp());
            continue;
        }

        if (orders[i].side == 1) {
            while (true){
                // new entries 
                // a new order to the order heap and a nex exec report is added
                if (sellHeap.empty() || sellHeap.top()->price > orders[i].price){
                    buyHeap.push(&orders[i]);
                    if (!partially_filled){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "New", orders[i].quantity, orders[i].price,"", getCurrentTimestamp());
                    }
                    break;
                }
                // executable orders
                else if (sellHeap.top()->price<=orders[i].price){
                    // current top order needs to be popped, two reports needed to be added
                    Order * popped = sellHeap.top();
                    sellHeap.pop();
                    if (popped->quantity == orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        break;
                    }
                    else if (popped->quantity > orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "PFill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        popped->quantity -= orders[i].quantity;
                        sellHeap.push(&orders[i]);
                        break;
                    }
                    else{
                        partially_filled = true;
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "PFill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        orders[i].quantity -= popped->quantity;
                        buyHeap.push(&orders[i]);
                    }
                }
            }
        } else if (orders[i].side == 2) {
            while(true){
                // new entries
                // a new order to the order heap and a nex exec report is added
                if (buyHeap.empty() || buyHeap.top()->price < orders[i].price){
                    sellHeap.push(&orders[i]);
                    if (!partially_filled){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "New", orders[i].quantity, orders[i].price,"", getCurrentTimestamp());
                    }
                    break;
                }
                // executable orders
                else if (buyHeap.top()->price>=orders[i].price){
                    // current top order needs to be popped, two reports needed to be added
                    Order * popped = buyHeap.top();
                    buyHeap.pop();
                    if (popped->quantity == orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        break;
                    }
                    else if (popped->quantity > orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "PFill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        popped->quantity -= orders[i].quantity;
                        buyHeap.push(&orders[i]);
                        break;
                    }
                    else{
                        partially_filled = true;
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "PFill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        orders[i].quantity -= popped->quantity;
                        sellHeap.push(&orders[i]);
                    }
                }
            }
        }
    }
    vector<string> attributes = {"Order ID", "Client Order ID", "Instrument", "Side", "Exec Status", "Quantity", "Price","Reason", "Time"};
    writeToCSVFile("exec_reports.csv", attributes, data);

    return;
}


int main() {
    vector<Order> orders = readCSVFile("orders.csv");
    handleOrders(orders);
    return 0;
}