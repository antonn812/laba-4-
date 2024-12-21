#include <iostream>
#include <pqxx/pqxx>
#include <fstream>
#include <vector>
#include <string>

using namespace std;
using namespace pqxx;

// Класс Product
class Product {
public:
    int id;
    string name;
    double price;

    Product() = default;
    Product(int id, const string& name, double price) : id(id), name(name), price(price) {}

    static void add_product(connection& conn, const string& name, double price) {
        try {
            work txn(conn);
            txn.exec("INSERT INTO products (name, price) VALUES (" + txn.quote(name) + ", " + to_string(price) + ")");
            txn.commit();
            cout << "Товар добавлен: " << name << " (Цена: " << price << ")\n";
        }
        catch (const exception& e) {
            cerr << "Ошибка: " << e.what() << endl;
        }
    }

    static void view_products(connection& conn) {
        try {
            work txn(conn);
            result res = txn.exec("SELECT * FROM products");
            cout << "Список товаров:\n";
            for (auto row : res) {
                cout << "ID: " << row["id"].as<int>() << ", Название: " << row["name"].c_str()
                    << ", Цена: " << row["price"].as<double>() << endl;
            }
        }
        catch (const exception& e) {
            cerr << "Ошибка: " << e.what() << endl;
        }
    }
};

// Класс Order
class Order {
public:
    int id;
    string date;

    static int add_order(connection& conn, const string& date) {
        try {
            work txn(conn);
            result res = txn.exec("INSERT INTO orders (order_date) VALUES (" + txn.quote(date) + ") RETURNING order_id");
            int order_id = res[0]["order_id"].as<int>();
            txn.commit();
            cout << "Заказ добавлен. ID заказа: " << order_id << "\n";
            return order_id;
        }
        catch (const exception& e) {
            cerr << "Ошибка: " << e.what() << endl;
            return -1;
        }
    }

    static void view_orders(connection& conn) {
        try {
            work txn(conn);
            result res = txn.exec("SELECT * FROM orders");
            cout << "Список заказов:\n";
            for (auto row : res) {
                cout << "ID: " << row["order_id"].as<int>() << ", Дата: " << row["order_date"].c_str() << endl;
            }
        }
        catch (const exception& e) {
            cerr << "Ошибка: " << e.what() << endl;
        }
    }
};

// Класс OrderItem
class OrderItem {
public:
    static void add_order_item(connection& conn, int order_id, int product_id, int quantity) {
        try {
            work txn(conn);
            txn.exec("INSERT INTO order_items (order_id, product_id, quantity, total_price) "
                "VALUES (" + to_string(order_id) + ", " + to_string(product_id) + ", " +
                to_string(quantity) + ", (SELECT price FROM products WHERE id = " +
                to_string(product_id) + ") * " + to_string(quantity) + ")");
            txn.commit();
            cout << "Позиция добавлена в заказ " << order_id << "\n";
        }
        catch (const exception& e) {
            cerr << "Ошибка: " << e.what() << endl;
        }
    }
};

// Аналитические запросы
void get_revenue_per_product(connection& conn) {
    try {
        work txn(conn);
        result res = txn.exec(
            "SELECT p.name, SUM(oi.total_price) AS revenue "
            "FROM products p JOIN order_items oi ON p.id = oi.product_id "
            "GROUP BY p.name ORDER BY revenue DESC");
        cout << "Выручка по каждому товару:\n";
        for (auto row : res) {
            cout << "Товар: " << row["name"].c_str() << ", Выручка: " << row["revenue"].as<double>() << endl;
        }
    }
    catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
    }
}

// Основное меню
void menu(connection& conn) {
    int choice;
    do {
        cout << "\nМеню:\n";
        cout << "1. Добавить товар\n";
        cout << "2. Просмотреть товары\n";
        cout << "3. Добавить заказ\n";
        cout << "4. Добавить позицию в заказ\n";
        cout << "5. Просмотреть заказы\n";
        cout << "6. Отчет по выручке\n";
        cout << "7. Выход\n";
        cout << "Выберите действие: ";
        cin >> choice;

        switch (choice) {
        case 1: {
            string name;
            double price;
            cout << "Введите название товара: ";
            cin >> name;
            cout << "Введите цену товара: ";
            cin >> price;
            Product::add_product(conn, name, price);
            break;
        }
        case 2:
            Product::view_products(conn);
            break;
        case 3: {
            string date;
            cout << "Введите дату заказа (YYYY-MM-DD): ";
            cin >> date;
            Order::add_order(conn, date);
            break;
        }
        case 4: {
            int order_id, product_id, quantity;
            cout << "Введите ID заказа: ";
            cin >> order_id;
            cout << "Введите ID товара: ";
            cin >> product_id;
            cout << "Введите количество: ";
            cin >> quantity;
            OrderItem::add_order_item(conn, order_id, product_id, quantity);
            break;
        }
        case 5:
            Order::view_orders(conn);
            break;
        case 6:
            get_revenue_per_product(conn);
            break;
        case 7:
            cout << "Выход из программы.\n";
            break;
        default:
            cout << "Некорректный выбор. Попробуйте снова.\n";
        }
    } while (choice != 7);
}

// Главная функция
int main() {
    try {
        connection conn("dbname=sales_db user=postgres password=123456 hostaddr=127.0.0.1 port=5432");
        if (conn.is_open()) {
            cout << "Успешное подключение к базе данных.\n";
            menu(conn);
        }
        else {
            cerr << "Не удалось подключиться к базе данных.\n";
        }
    }
    catch (const exception& e) {
        cerr << "Ошибка: " << e.what() << endl;
    }
    return 0;
}
