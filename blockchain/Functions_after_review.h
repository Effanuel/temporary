#pragma once
#include "Blockchain.h"
#include "User.h"

/// Generates a vector of unconfirmed transactions with class 'Transaction'
vector<User> generateUsers(unsigned int number) {
    vector<User> users;
    users.reserve(number);
    for (unsigned int i = 0; i < number; ++i) {
        users.push_back(User{});
    }
    return users;
}

vector<Transaction> validateTransactions(vector<User>& blockchain_Users, vector<Transaction>& unvalidated_Transactions) {
    using Wallets_users = std::map<string, User>;

    /// Validate based on previous and after hashes
    std::remove_if(unvalidated_Transactions.begin(), unvalidated_Transactions.end(),[&](Transaction& trans) {
		return trans.checkValidity();
	});

    /// Create map of users and their wallets
    Wallets_users wallet_users;
    for (const auto& user : blockchain_Users) {
        wallet_users.insert(std::pair<string, User>(user.walletId, user));
    }

    vector<Transaction> validated_Transactions;
    Wallets_users::const_iterator pos;
    /// Iterate over non-validated transactions
    for (const auto& trans : unvalidated_Transactions) {
        /// Get position of map of transactions sender wallet
        pos = wallet_users.find(trans.senderWallet);

        /// Check if transactinos exists
        /// AND
        /// Using sender wallet, get User and check whether balance >=
        /// transaction sender amount
        if (pos != wallet_users.end() && wallet_users[pos->first].getBalance() >= trans.amount) {
            validated_Transactions.push_back(trans);
        }
    }
    /// Return validated transactions
    return validated_Transactions;
}

/// Generates a vector of unconfirmed transactions with class 'Transaction'
/// and sets it to 'Blockchain' unconfirmed transactions
void generateTransactions(Blockchain& blockchain, unsigned int numberOfTransactions) {
    vector<User> blockchain_Users = blockchain.getUsers();

    std::random_device device;
    static std::mt19937 gen(device());
    std::uniform_int_distribution<int> distr(0, blockchain_Users.size() - 1);
    std::uniform_real_distribution<double> distr_amount(5.0, 50.0);
    std::uniform_real_distribution<double> distr_fee(0.0, 2.0);

    vector<Transaction> unvalidatedTransactions;
    unvalidatedTransactions.reserve(numberOfTransactions);

    unsigned int senderID;
    for (unsigned int i = 0; i < numberOfTransactions; ++i) {
        senderID = distr(gen);

        unvalidatedTransactions.push_back(Transaction{
            blockchain_Users[senderID].walletId,
            blockchain_Users[distr(gen)].walletId,
            blockchain_Users[senderID].getBalance() * 0.2, distr_fee(gen)});
    }

    /// Validated transactions based on balance/sent amount check
    vector<Transaction> vT = validateTransactions(blockchain_Users, unvalidatedTransactions);

    std::cout << "Deleted " << numberOfTransactions - vT.size() << " invalid transactions." << std::endl;

    /// Add validated(with hashes and balances) transactions to unconfirmed transaction pool
    blockchain.addTransactions(vT);
    unvalidatedTransactions.clear();
}
