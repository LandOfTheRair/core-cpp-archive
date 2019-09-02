template<typename T>
concept bool DatabaseTransaction = requires(T a) {
{ a.commit() } -> void;
};

template<typename T>
concept bool UserRepository = requires(T a, DatabaseTransaction transaction) {
{ a.insert_user_if_not_exists(transaction) } -> void;
};

int main() {
    return 0;
}