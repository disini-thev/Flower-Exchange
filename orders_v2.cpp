#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <queue>
#include <unordered_map>

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

class OrderBook {
public:
    string name;
    priority_queue<Order*, vector<Order*>, MaxHeapComparator> buyHeap;
    priority_queue<Order*, vector<Order*>, MinHeapComparator> sellHeap;

    OrderBook(string name) {
        this->name = name;
        this->buyHeap = priority_queue<Order*, vector<Order*>, MaxHeapComparator>();
        this->sellHeap = priority_queue<Order*, vector<Order*>, MinHeapComparator>();
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

// Function to write the data to a CSV file
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

// Helper Functions

// Function to get the current timestamp in the format YYYYMMDD-HHMMSS.mmm
string getCurrentTimestamp() {
    // Get the current time point
    auto now = chrono::system_clock::now();

    // Convert the time point to a time_t object
    time_t time = chrono::system_clock::to_time_t(now);

    // Convert time_t to a struct tm in UTC
    tm* tm_utc = gmtime(&time);
    // for Sri Lankan Time
    tm_utc->tm_hour+=5;
    tm_utc->tm_min+=30;

    // Create a time string with the desired format
    ostringstream oss;
    oss << put_time(tm_utc, "%Y%m%d-%H%M%S");
    
    // Get milliseconds and append to the timestamp
    auto milliseconds = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    oss << '.' << setfill('0') << setw(3) << milliseconds.count();

    return oss.str();
}

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

// initialization of orderbooks
unordered_map<string, OrderBook*> initializeOrderBooks() {
    unordered_map<string, OrderBook*> books;
    vector<string> instruments = {"Rose", "Lavender", "Lotus", "Tulip", "Orchid"};
    for (const auto& instrument : instruments) {
        books[instrument] = new OrderBook(instrument);
    }
    return books;
}

// Exchange Function
void handleOrders (vector<Order> &orders, vector<Exec_report> &data){
    // cout << "Handling orders..." << endl;
    // dictionary of orderbooks for different types of instruments
    unordered_map<string, OrderBook*> orderBooks = initializeOrderBooks();
    // a vector of exec reports

    for (int i=0; i<orders.size(); i++) {
        bool partially_filled = false; // used to check if the order needs to be enterred to the orderbook as a new entry
        // data validation
        string ret = validateData(orders[i]);
        // cout << ret << endl;
        
        // Invalid entries enterred as Reject Exec Reports
        if (ret !=""){
            data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Reject", orders[i].quantity, orders[i].price,ret, getCurrentTimestamp());
            continue;
        }
        
        // current instrument
        string instrument = orders[i].Instrument;
        if (orders[i].side == 1) {
            while (true){
                // new entries 
                // a new order to the order heap and a nex exec report is added
                if (orderBooks[instrument]->sellHeap.empty() || orderBooks[instrument]->sellHeap.top()->price > orders[i].price){
                    orderBooks[instrument]->buyHeap.push(&orders[i]);
                    if (!partially_filled){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "New", orders[i].quantity, orders[i].price,"", getCurrentTimestamp());
                    }
                    break;
                }
                // executable orders
                else if (orderBooks[instrument]->sellHeap.top()->price <= orders[i].price){
                    // current top order needs to be popped, two reports needed to be added
                    Order * popped = orderBooks[instrument]->sellHeap.top();
                    orderBooks[instrument]->sellHeap.pop();
                    if (popped->quantity == orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        break;
                    }
                    else if (popped->quantity > orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "PFill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        popped->quantity -= orders[i].quantity;
                        orderBooks[instrument]->sellHeap.push(&orders[i]);
                        break;
                    }
                    else{
                        partially_filled = true;
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "PFill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        orders[i].quantity -= popped->quantity;
                        orderBooks[instrument]->buyHeap.push(&orders[i]);
                    }
                }
            }
        } else if (orders[i].side == 2) {
            while(true){
                // new entries
                // a new order to the order heap and a nex exec report is added
                if (orderBooks[instrument]->buyHeap.empty() || orderBooks[instrument]->buyHeap.top()->price < orders[i].price){
                    orderBooks[instrument]->sellHeap.push(&orders[i]);
                    if (!partially_filled){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "New", orders[i].quantity, orders[i].price,"", getCurrentTimestamp());
                    }
                    break;
                }
                // executable orders
                else if (orderBooks[instrument]->buyHeap.top()->price>=orders[i].price){
                    // current top order needs to be popped, two reports needed to be added
                    Order * popped = orderBooks[instrument]->buyHeap.top();
                    orderBooks[instrument]->buyHeap.pop();
                    if (popped->quantity == orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        break;
                    }
                    else if (popped->quantity > orders[i].quantity){
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "Fill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "PFill", orders[i].quantity, popped->price,"", getCurrentTimestamp());
                        popped->quantity -= orders[i].quantity;
                        orderBooks[instrument]->buyHeap.push(&orders[i]);
                        break;
                    }
                    else{
                        partially_filled = true;
                        data.emplace_back(orders[i].Order_ID, orders[i].Client_ID, orders[i].Instrument, orders[i].side, "PFill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        data.emplace_back(popped->Order_ID, popped->Client_ID, popped->Instrument, popped->side, "Fill", popped->quantity, popped->price,"", getCurrentTimestamp());
                        orders[i].quantity -= popped->quantity;
                        orderBooks[instrument]->sellHeap.push(&orders[i]);
                    }
                }
            }
        }
    }
    

    /*
    // Print the number of entries in each orderbook
    for (const auto& pair : orderBooks) {
        const string& instrument = pair.first;
        const OrderBook* orderBook = pair.second;
        cout << "OrderBook: " << instrument << endl;
        cout << "BuyHeap entries: " << orderBook->buyHeap.size() << endl;
        cout << "SellHeap entries: " << orderBook->sellHeap.size() << endl;
    }
    */
    return;
}


int main() {
    vector<Order> orders = readCSVFile("orders.csv");
    vector<Exec_report> exec_reports;
    handleOrders(orders,exec_reports);
    vector<string> attributes = {"Order ID", "Client Order ID", "Instrument", "Side", "Exec Status", "Quantity", "Price","Reason", "Time"};
    writeToCSVFile("exec_reports.csv", attributes, exec_reports);
    return 0;
}