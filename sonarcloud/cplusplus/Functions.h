#pragma once
#include "Blockchain.h"
#include "User.h"



///Generates a vector of unconfirmed transactions with class 'Transaction'
vector<User> generateUsers(unsigned int number) {
	vector<User> users;
	users.reserve(number);
	for (unsigned int i = 0; i < number; ++i) {
		users.push_back(User{});
	}
	return users;
}

vector<Transaction> validateTransactions(vector<User>& blockchain_Users, vector<Transaction>& unT) {
	///
	typedef std::map<string, User> MAP_wallet_user;
	///

	/// Validate based on previous and after hashes
	std::remove_if(unT.begin(), unT.end(),
		[&](Transaction & trans) { return trans.checkValidity(); });


	/// Create map of users and their wallets
	MAP_wallet_user wallet_user;
	for (const auto& user : blockchain_Users) {
		wallet_user.insert(std::pair<string, User>(user.walletId, user));
	}


	vector<Transaction> validated_Transactions;
	MAP_wallet_user::const_iterator pos;
	/// Iterate over non-validated transactions
	for (const auto& trans : unT) {
		
		/// Get position of map of transactions sender wallet
		pos = wallet_user.find(trans.senderWallet);

		/// Check if transactinos exists 
		/// AND
		/// Using sender wallet, get User and check whether balance >= transaction sender amount
		//std::cout << wallet_user[pos->first].getBalance() << std::endl;
		//std::cout << "Balance" << trans.amount << std::endl;
		if (pos != wallet_user.end() && wallet_user[pos->first].getBalance() >= trans.amount)
			validated_Transactions.push_back(trans);
			
	}
	/// Return validated transactions
	return validated_Transactions;
}


///Generates a vector of unconfirmed transactions with class 'Transaction'
///and sets it to 'Blockchain' unconfirmed transactions
void generateTransactions(Blockchain& blockchain, unsigned int numberOfTransactions) {


	vector<User> blockchain_Users = blockchain.getUsers();

	std::random_device device;
	static std::mt19937 gen(device()); // CHANGE LATER
	//gen.seed(std::random_device()());
	static std::uniform_int_distribution<int> distr(0, blockchain_Users.size() - 1); //error???
	static std::uniform_real_distribution<double> distr_amount(5.0, 50.0);
	static std::uniform_real_distribution<double> distr_fee(0.0, 2.0);

	vector<Transaction> unT;
	unT.reserve(numberOfTransactions);

	unsigned int senderID;
	for (unsigned int i = 0; i < numberOfTransactions; ++i) {

		senderID = distr(gen);

		unT.push_back(
			Transaction{
				blockchain_Users[senderID].walletId,			/// SENDER WALLET
				blockchain_Users[distr(gen)].walletId,			/// RECEIVER WALLET
				blockchain_Users[senderID].getBalance()*0.2,	/// SEND ONLY 20% OF BALANCE
				distr_fee(gen) });								/// FEE
	}


	/// Validated transactions based on balance/sent amount check
	vector<Transaction> vT = validateTransactions(blockchain_Users, unT);


	std::cout << "Deleted " << numberOfTransactions - vT.size() <<
		" invalid transactions." << std::endl;


	/// Add validated(with hashes and balances) transactions to unconfirmed transaction pool
	blockchain.addTransaction(vT);
	unT.clear();
}







