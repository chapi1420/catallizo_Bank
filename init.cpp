#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <functional> 
#include <ctime>      
#include <filesystem> 
using namespace std;

// Account data structure
struct AccountData {
    char phoneNumber[15]; 
    char password[50];    
    char preferredName[50];
    double balance;       
};

// Transaction data structure
struct Transaction {
    char type[20];        
    double amount;        
    char timestamp[30];   
};

// Hash the password
string hashPassword(const string& password) {
    hash<string> hasher;
    return to_string(hasher(password));
}

// Check if an account exists
bool accountExists(const string& phoneNumber) {
    string filename = "credentials/" + phoneNumber + ".dat";
    ifstream file(filename, ios::binary);
    return file.good();
}

// Create a new account
void createAccount(const string& phoneNumber, const string& password, const string& preferredName) {
    // Create credentials directory if it doesn't exist
    filesystem::create_directory("credentials");

    string filename = "credentials/" + phoneNumber + ".dat";

    AccountData account;
    memset(&account, 0, sizeof(AccountData)); // Initialize struct to 0

    strncpy(account.phoneNumber, phoneNumber.c_str(), sizeof(account.phoneNumber) - 1);
    string hashedPassword = hashPassword(password);
    strncpy(account.password, hashedPassword.c_str(), sizeof(account.password) - 1);
    strncpy(account.preferredName, preferredName.c_str(), sizeof(account.preferredName) - 1);
    account.balance = 0.0;

    // Save account to binary file
    ofstream outFile(filename, ios::binary);
    if (outFile) {
        outFile.write(reinterpret_cast<char*>(&account), sizeof(AccountData));
        cout << "Account created successfully!\n";
    } else {
        cerr << "Error creating account!\n";
    }
}
// Login to an account
bool login(const string& phoneNumber, const string& password, string& preferredName) {
    string filename = "credentials/" + phoneNumber + ".dat";

    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "Account does not exist!\n";
        return false;
    }

    AccountData account;
    file.read(reinterpret_cast<char*>(&account), sizeof(AccountData));

    // Log-in password
    string hashedPassword = hashPassword(password);
    for (int i = 0; i < 3; ++i) {
        if (hashedPassword == account.password) {
            preferredName = account.preferredName;
            cout << "Login successful!\n Hello, " << preferredName << "!\n";
            return true;
        } else {
            cout << "Incorrect password!\n";
            cout << "You have " << 2 - i << " chances left.\n";
        }
    }
    return false;
}

// Verify sender's password
bool verifyPassword(const string& phoneNumber, const string& password) {
    string filename = "credentials/" + phoneNumber + ".dat";

    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "Account does not exist!\n";
        return false;
    }

    AccountData account;
    file.read(reinterpret_cast<char*>(&account), sizeof(AccountData));

    string hashedPassword = hashPassword(password);
    if (hashedPassword == account.password) {
        return true; 
    } else {
        cout << "Verification failed. Incorrect password.\n";
        return false;
    }
}

// Verify receiver's preferred name
bool verifyReceiverName(const string& receiverPhone, const string& preferredName) {
    string filename = "credentials/" + receiverPhone + ".dat";

    ifstream file(filename, ios::binary);
    if (!file) {
        cout << "Receiver's account does not exist!\n";
        return false;
    }

    AccountData account;
    file.read(reinterpret_cast<char*>(&account), sizeof(AccountData));

    if (preferredName == account.preferredName) {
        return true; 
    } else {
        cout << "Verification failed. Incorrect receiver's preferred name.\n";
        return false;
    }
}

// Update account balance (deposit or withdraw)
void updateBalance(const string& phoneNumber, double amount, bool isDeposit, const string& transactionType = "") {
    string filename = "credentials/" + phoneNumber + ".dat";

    fstream file(filename, ios::in | ios::out | ios::binary);
    if (!file) {
        cerr << "Error opening account file!\n";
        return;
    }

    AccountData account;
    file.read(reinterpret_cast<char*>(&account), sizeof(AccountData));

    if (isDeposit) {
        account.balance += amount;
    } else {
        if (amount > account.balance) {
            cout << "Insufficient balance!\n";
            return;
        }
        account.balance -= amount;
    }

    file.seekp(0); 
    file.write(reinterpret_cast<char*>(&account), sizeof(AccountData));

    // Record transaction
    if (!transactionType.empty()) {
        // Create transactions directory if it doesn't exist
        filesystem::create_directory("transactions");

        string transactionFilename = "transactions/" + phoneNumber + "_transactions.dat";

        Transaction transaction;
        strncpy(transaction.type, transactionType.c_str(), sizeof(transaction.type) - 1);
        transaction.amount = amount;

        // Get current timestamp
        time_t now = time(0);
        strftime(transaction.timestamp, sizeof(transaction.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

        // Save transaction to binary file
        ofstream transactionFile(transactionFilename, ios::app | ios::binary);
        if (transactionFile) {
            transactionFile.write(reinterpret_cast<char*>(&transaction), sizeof(Transaction));
        } else {
            cerr << "Error recording transaction!\n";
        }
    }
}

// Check account balance
double checkBalance(const string& phoneNumber) {
    string filename = "credentials/" + phoneNumber + ".dat";

    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Error opening account file!\n";
        return -1;
    }

    AccountData account;
    file.read(reinterpret_cast<char*>(&account), sizeof(AccountData));

    return account.balance;
}

// Transfer money from one account to another
void transfer(const string& senderPhone, const string& receiverPhone, double amount, const string& senderPassword, const string& receiverPreferredName) {
    // Verify sender's password
    if (!verifyPassword(senderPhone, senderPassword)) {
        return;
    }

    // Verify receiver's preferred name
    if (!verifyReceiverName(receiverPhone, receiverPreferredName)) {
        return;
    }

    // Check if receiver's account exists
    if (!accountExists(receiverPhone)) {
        cout << "Error: The receiver's account does not exist. Please check the phone number and try again.\n";
        return;
    }

    // Check if sender has sufficient balance
    double senderBalance = checkBalance(senderPhone);
    if (senderBalance < amount) {
        cout << "Error: Insufficient balance. You cannot transfer more than your current balance.\n";
        return;
    }

    // Withdraw from sender
    updateBalance(senderPhone, amount, false, "transfer-out");

    // Deposit to receiver
    updateBalance(receiverPhone, amount, true, "transfer-in");

    cout << "Transfer successful!\n";
}

// View transaction history
void viewTransactionHistory(const string& phoneNumber) {
    string transactionFilename = "transactions/" + phoneNumber + "_transactions.dat";

    ifstream transactionFile(transactionFilename, ios::binary);
    if (!transactionFile) {
        cout << "No transactions found!\n";
        return;
    }

    cout << "Transaction History for " << phoneNumber << ":\n";
    Transaction transaction;
    while (transactionFile.read(reinterpret_cast<char*>(&transaction), sizeof(Transaction))) {
        cout << "Type: " << transaction.type << ", Amount: " << transaction.amount
             << ", Timestamp: " << transaction.timestamp << "\n";
    }
}

// Main program
int main() {
    string phoneNumber, password, preferredName;
    double amount;

    while (true) {
        // First experience: Enter phone number
        cout << "Welcome to the Catallizo wallet System!\n";
        cout << "Enter your phone number: ";
        cin >> phoneNumber;

        if (accountExists(phoneNumber)) {
            // Login
            cout << "Account found. Please log in.\n";
            cout << "Enter your password: ";
            cin >> password;

            if (login(phoneNumber, password, preferredName)) {
                // Logged in successfully
                int choice;
                while (true) {
                    cout << "\n1. Deposit\n2. Withdraw\n3. Check Balance\n4. Transfer\n5. View Transaction History\n6. Exit\n";
                    cin >> choice;

                    switch (choice) {
                        case 1: {
                            cout << "Enter amount to deposit: ";
                            cin >> amount;
                            updateBalance(phoneNumber, amount, true, "deposit");
                            cout << "Deposit successful!\n";
                            cout << "....****....****....****....\n";
                            break;
                        }

                        case 2: {
                            cout << "....****....****....****....\n";
                            cout << "Enter amount to withdraw: ";
                            cin >> amount;
                            cout << "Verify your identity. Enter your password: ";
                            string senderPassword;
                            cin >> senderPassword;
                            if (verifyPassword(phoneNumber, senderPassword)) {
                                updateBalance(phoneNumber, amount, false, "withdraw");
                                cout << "Withdraw successful!\n";
                            }
                            cout << "....****....****....****....\n";
                            break;
                        }

                        case 3:
                            cout << "Your balance is: $" << checkBalance(phoneNumber) << "\n";
                            break;

                        case 4: {
                            cout << "....****....****....****....\n";
                            string receiverPhone;
                            cout << "Enter receiver's phone number: ";
                            cin >> receiverPhone;
                            cout << "Enter amount to transfer: ";
                            cin >> amount;
                            cout << "Verify your identity. Enter your password: ";
                            string senderPassword;
                            cin >> senderPassword;
                            cout << "Verify receiver's identity. Enter receiver's preferred name: ";
                            string receiverPreferredName;
                            cin >> receiverPreferredName;
                            transfer(phoneNumber, receiverPhone, amount, senderPassword, receiverPreferredName);
                            cout << "....****....****....****....\n";
                            break;
                        }

                        case 5:
                            cout << "....****....****....****....\n";
                            viewTransactionHistory(phoneNumber);
                            cout << "....****....****....****....\n";
                            break;

                        case 6:
                            cout << "Exiting...\n";
                            return 0;

                        default:
                            cout << "Invalid choice!\n";
                    }
                }
            }
        } else {
            // Create new account
            cout << "Account not found. Create a new account.\n";
            cout << "Enter a password: ";
            cin >> password;
            cout << "Enter your preferred name: ";
            cin >> preferredName;
            createAccount(phoneNumber, password, preferredName);

            // After creating the account, proceed to login
            cout << "Account created successfully! Redirecting to login...\n";
        }
    }

    return 0;
}