

#include <map>
#include <cassert>
#include <iostream>
#include <functional>

/// Бесконечная матрица
template <typename T>
struct Matrix 
{
  /// Конструктор
  Matrix(T def) : m_def_value(std::move(def)) {}
  /// Вернуть реальный размер матрицы
  size_t size() const { return m_size; };

  struct my_iterator {
    friend struct Matrix;

    struct slice {
      slice(int row, int column, T*p) : m_row(row), m_column(column), m_ptr_value(p) {}
      operator std::tuple<int&, int&, T&>() {
        std::tuple<int&, int&, T&> foo(std::ref(m_row), std::ref(m_column), std::ref(*m_ptr_value));
        return foo;
      }
      int m_row{-1}, m_column{-1};
      T* m_ptr_value{nullptr};
    };
    
    bool operator != (const my_iterator & it) const {  return m_index != it.m_index; }
    slice operator * () { 
      assert(m_cur_value != nullptr); 
      return slice(m_it_row->first, m_cur_it_column->first, m_cur_value);
    }

    void operator++ () { 
      m_index++;
      assert(m_cur_value != nullptr);
      m_cur_it_column++;
      if ( m_cur_it_column == m_end_it_column ) {
        m_it_row++;
        if ( m_it_row != m_ptr->m_data.end() ) {
          m_cur_it_column = m_it_row->second.begin();
          m_end_it_column = m_it_row->second.end();
          assert(m_cur_it_column != m_end_it_column);
          m_cur_value = &m_cur_it_column->second;
        } else {
          m_cur_value = nullptr;
        }
      } else {
        m_cur_value = &m_cur_it_column->second;
      }
    }
    
  private:
    /// Конструктор
    my_iterator(Matrix*p, bool first) : m_ptr(p)  {
      if ( first ) {
        m_index = 0;   
        m_it_row = m_ptr->m_data.begin();
        if ( m_it_row != m_ptr->m_data.end() ) {
          m_cur_it_column = m_it_row->second.begin();
          m_end_it_column = m_it_row->second.end();
          assert(m_cur_it_column != m_end_it_column);
          m_cur_value = &m_cur_it_column->second;
        }
      } else {
        m_index = m_ptr->size();
        m_it_row = m_ptr->m_data.end();
      }
    };
    /// указатель на главный класс
    Matrix<T> *m_ptr{nullptr};
    T* m_cur_value{nullptr};
    
    size_t m_index{0};
    typename std::map<int, std::map<int, T> >::iterator m_it_row;
    typename std::map<int, T>::iterator m_cur_it_column; 
    typename std::map<int, T>::iterator m_end_it_column;
  };

  my_iterator  begin() { return my_iterator(this, true); }
  my_iterator  end()   { return my_iterator(this, false); }

  /// Прокси структура, для хранения временного состояния
  struct proxy {  
    friend struct Matrix;
    /// сохранить номер колонки 
    proxy operator [] (int number) {      
      m_column = number; // запоминаем номер колонки
      assert(m_ptr != nullptr);   assert(m_column >= 0);     assert(m_row >= 0);
      return *this;
    }
    /// проверить значение в матрице
    bool operator == (const T &v) {
      assert(m_ptr != nullptr);   assert(m_column >= 0);
      // ищем по строкам
      auto it_row = m_ptr->m_data.find(m_row);
      if ( it_row != m_ptr->m_data.end() ) {
        // ищем по столбцам
        auto it_column = it_row->second.find(m_column);
        if ( it_column != it_row->second.end() ) {
          return v == it_column->second;
        }
      }
      return v == m_ptr->m_def_value;
    }

    /// сохранить новое значение в матрице
    /// если value = -1, то удалить значение из матрицы, если оно там есть
    template <typename C>
    void operator = (C&& value) {            
      assert(m_ptr != nullptr);   assert(m_column >= 0);     assert(m_row >= 0);
      if ( value == m_ptr->m_def_value ) { // удаляем, если найдем
        // ищем по строкам
        auto it_row = m_ptr->m_data.find(m_row);
        if ( it_row != m_ptr->m_data.end() ) {
          // ищем по столбцам
          auto it_column = it_row->second.find(m_column);
          if ( it_column != it_row->second.end() ) {
            it_row->second.erase(it_column);   // удаляем
            m_ptr->m_size--;
          }
          if ( it_row->second.empty() )
            m_ptr->m_data.erase(it_row);
        }
      } else {  // вставляем новое значение, или заменяем, если уже там что-то есть
        std::map<int, T>&col_to_v = m_ptr->m_data[m_row];
        auto size = col_to_v.size();
        col_to_v[m_column] = std::forward<T>(value);
        if ( size != col_to_v.size() )
          m_ptr->m_size++; // увеличеваем размер, если там ничего не было
      }
    }
    
  private:
    /// Конструктор. Запоминаем номер строки
    proxy(Matrix*p, int r) : m_ptr(p), m_row(r) {};
    /// указатель на главный класс
    Matrix<T> *m_ptr{nullptr};
    /// номер строки и номер колонки
    int m_row{-1}, m_column{-1};
  };

  /// создать объект прокси и запоминть номер строки в нем
  proxy operator [] (int number) {
    proxy d(this, number);
    return d;
  }

private:  
  T m_def_value;
  std::map<int, std::map<int, T> > m_data;
  size_t m_size{0};
};


int main()
{
  Matrix<int> matrix(-1); 

  assert(matrix.size() == 0);
  auto a = matrix[0][0];
  assert(a == -1);
  assert(matrix.size() == 0);

  matrix[100][100] = 314;
  assert(matrix[100][100] == 314);
  assert(matrix.size() == 1);

  matrix[100][100] = 315;
  assert(matrix[100][100] == 315);
  assert(matrix.size() == 1);

  matrix[100][100] = -1;
  assert(matrix.size() == 0);

  matrix[100][100] = -1;
  assert(matrix.size() == 0);

  matrix[1000][10] = 23;
  assert(matrix[1000][10] == 23);
  assert(matrix.size() == 1);

  matrix[10][1000] = 12;
  assert(matrix[10][1000] == 12);
  assert(matrix.size() == 2);

  matrix[100][100] = 314;
  assert(matrix[100][100] == 314);
  assert(matrix.size() == 3);

  matrix[100][10] = 89;
  assert(matrix[100][10] == 89);
  assert(matrix.size() == 4);

  matrix[100][10] = -1;
  assert(matrix[100][10] == -1);
  assert(matrix.size() == 3);

  matrix[100][100] = -1;
  assert(matrix[100][100] == -1);
  assert(matrix.size() == 2);


  matrix[1000][10] = -1;
  assert(matrix[1000][10] == -1);
  assert(matrix.size() == 1);

  matrix[10][1000] = -1;
  assert(matrix[10][1000] == -1);
  assert(matrix.size() == 0);

  matrix[100][100] = 314;
  assert(matrix[100][100] == 314);
  assert(matrix.size() == 1);

  // выведется одна строка
  // 100100314

  for ( auto c : matrix ) {
    int x;
    int y;
    int v;
    std::tie(x, y, v) = c;
    std::cout << x << y << v << std::endl;
  }  
  Matrix<char> matrix_char('a');
  matrix_char[100][100] = 'b';
  assert(matrix_char[100][100] == 'b');
  matrix_char[100][100] = 'a';
  assert(matrix_char.size() == 0);
  for ( auto d : matrix_char ) {
    int x;
    int y;
    char v;
    std::tie(x, y, v) = d;
    std::cout << x << y << v << std::endl;
  }
  Matrix<std::string> matrix_strings(std::string("hi"));
  auto tmp = matrix_strings[2][2];
  tmp = "hello";
  assert(tmp == "hello");
  assert(matrix_strings.size() == 1);
  assert(matrix_strings[2][2] == "hello");
  tmp = "hi";
  assert(matrix_strings.size() == 0);
  for ( auto d : matrix_strings ) {
    int x;
    int y;
    std::string v;
    std::tie(x, y, v) = d;
    std::cout << x << y << v.c_str() << std::endl;
  }
  return 0;
}
