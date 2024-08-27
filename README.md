# Что это и зачем так вышло?
Этот проект изначально был написан не мной и предназначался для исследования эффекта от различных кардионасосов на функцию сердца. Он строился в значительной степени вокруг фреймворка QT.

Внутри идет расчет одномерной модели большого круга кровообращения с левым сердцем и соответсвующими граничными условиями. Структура графа сосудов, их параметры и граничные условия описаны в [статье](https://pubmed.ncbi.nlm.nih.gov/26100764/) (смотри в район Figure 10)

Местами код выглядит так, как будто писали его "чужие :alien: для хищников :wolf:". В свое оправдание скажу, что и мне это досталось мне в наследство.

Теперь в проекте нет никакого QT _(тьфу-тьфу-тьфу)_, особо ставить ничего не надо, все собирается с помощью cmake и запускается из консоли. Я запускал это на Linux, при наличии Mac не должно быть проблем никаких. С Windows вероятно будут сложности, но любая IDE через такую-то матерь и пару приседаний должна этот проект собрать.

Если на Windows совсем никак, то рекомендую поставить [виртуальную машины](https://www.virtualbox.org/) с каким-то легеньких Linux.

Изменилась и цель проекта. Сейчас это код, предназначенный для подбора параметров под конкретного _"виртуального"_ пациента. На практике мы вводим возраст, систолическое давление, диастолическое давление, ударный объем и ЧСС, затем программа медленно, но верно с помощью алгоритма _Unscented Kalman Filter_ выбирает нужные нам параметры для достижения требуемых показателей.

# Очень интересно конечно, но как это все собрать?
После скачивания необходмо в корне создать пустую папку _/build_, она носит технический характер, туда будут складываться файлы сборки.

Далее самый простой способ - вызвать _bash-скрипт_ _./build.sh_ прямо из терпинала. Иначе в консоль можно ввести 
```cmake -DCMAKE_BUILD_TYPE=Release CMakeLists.txt && cmake --build . --parallel 8```

После этого в папке должен появиться исполняемый файл _bloodflow_.

# А запустить?

## Сначала запускаем без подбора
К сожалению _(или к счастью)_, это приложение консольное. Это позволяет удобно запускать его на кластере, и в целом там сложилась жизнь.

Любознательный читатель этой мини-документации, слегка знакомый с C++, легко сможет найти все возможные опции для запуска в файле ```main.cpp```. Они лежат в ```std::string mode```. 
А для запуска в терминал надо ввести ```./bloodflow -option_name option_param_1 option_param_2```. После ```-``` следует название опции, затем перечисляются требуемые параметры. Их может быть сколько угодно.

Для начала следует использовать опцию ```./bloodflow -detailed age HR data/back/base.csv```, где age - значение возраста, которое модет принимать значения от 25 до 75 включительно с шагом в 10 лет, HR - частота сердечных сокращений. Следующий параметр - это путь до стандартного набора параметров, на которые раньше была настроена модель.

В результате выполнения программы в папке out появятся csv файлы с данными, имя файла - будет соответствовать имени сосуда или Windkessel поверхности. Для каждого сосуда выводится следующая информация:
```
virtual void print_info(std::ostream &os) final {
        if (T >= 9.0 * this -> Period && T <= 10.0 * this -> Period) {
            os << T << "," << this -> get_center_pressure() * bar_to_mmhg <<
                "," << this -> get_center_speed() <<
                "," << this -> get_center_area() <<
                "," << this -> get_center_flow() <<
                "," << this -> get_mean_center_flow() << std::endl;
        }
    }
```
т.е. через запятую для одно цикла с девятого по десятый будут записаны давление, скорость потока, площадь просвета, поток и усредненный за цикл поток. 

Для сердца создается отдельный файл, куда данные выводятся по следующему правилу:
```
void print_info(std::ostream &os) {
        if ((T > 9.0 * Period) && (T < 10.0 * Period)) {
            os << T << "," << y0(q_av) << "," << y0(p_vent) << "," << y0(v_v) << "," << y0(ao_valve) << ","
               << y0(q_mi) << "," << y0(p_atri) << "," << y0(v_a) << "," << y0(mi_valve) << std::endl;
        }
    }
```
т.е. через запятую для одно цикла с девятого по десятый будут записаны поток через аортальный клапан, давление в левом желудочке, объем левого желудочка, угол раскрытия аортального клапана, поток через митральный клапан, давление в левом предсердии, объем левого предсердия, угол раскрытия митрального клапана.

Имея эти данные можно воспользоваться _gnuplot-ом_ или _python-ом_ и настроить разнообразные графики.
Про Windkessel поверхности можно найти аналогичную информацию, но пока это не представляет особого интереса.

## Теперь запускаем с подбором
Для этого используем следующую опцию ```./bloodflow -manual age HR SBP DBP SV HR```. В качестве параметров указываем возраст (с 25 до 75 с шагом 10) чсс, систолическое давление, диастолическое, ударный объем, ЧСС.

Дальше будет идти подбор параметров, который может продолжать довольно долго. Суть процесса описана файле ```ukf.cpp```, где реализован _Unscented Kalman Filter_. О том что и как примерно там изменяется можно посмотреть в [черновике статьи](https://cloud.mail.ru/public/pxdG/eJRhL1yed).

После запуска будет создан log-файл папке _data/log_ с информацией о том как идет подбор параметров.

## А что с остальными опциями?
Да, опций там больше, но по сути они являются обертками над двумя предыдущими. Разница лишь в том, что руками я не хочу каждый раз вводить целевые давления и ударные объемы, а хотелось бы создать конфиг файл, где все перечислено, отправить со спокойной душой на кластер и только результаты собирать.

Да, еще есть опция связанная с байесовской оптимизацией. Там все несколько сложнее, т.к. есть еще и отдельно стоящий для этого кусок на _python-е_. Пока время не пришло, пока оставляем за скобками :smile:

# Коротко о том, что и в каких файлах
- _main.cpp_ - входная точка приложения.
- _ukf.h ukf.cpp_ - класс, ответсвенный за подбор параметров с помощью фильтра Калмана.
- _task.h task.cpp_ - важный класс, который определяет расчетную задачу. Там происходит опеределение параметров сердца и сосудов из json конфигов.
- _matrix_utils.h matrix_utils.cpp_ - вспомогательный класс для перекладываения csv файлов в матрицы и обратно.
- _bayes_runner.h bayes_runner.cpp_ - класс для запуска подбора параметров с помощью байесовской оптимизации.
- _vertex.h vertex.cpp_ - класс, описывающий "вершину" графа сосудов или скорее некоторые граничные условия. Среди них внутренние точки соединения сосудов и соединение с "упрощенными сердца", где нет отдельной модели сердца, а просто функция _Q(t)_.
- _edge.h edge.cpp_ - класс, описывающий ребро графа сосудов (стало быть сосуд). Тут и его параметры, и численная схема, и запись результатов.
- _graph.h graph.cpp_ - класс, соединяющий сосуды с их вершинами. Особенно нигде явно не используется.
- _calculator.h calculator.cpp_ - вспомогательный класс для расчета средних величин в течение одного сердечного цикла.
- _rescaler.h_ - класс, который отвечает за установление ряда параметров, которые подбираются фильтром Калмана. Например, там подбирается суммарная емкость и сопротивление элементов Виндкесселя. Их надо перерасчитать на каждый отдельный элемент по определенному и записать в конфиг. Только после этого задача может быть корректно запущена. Да, это странная деталь реализации, просто получается, что проще все поменять в конфиге, а из него уже класс Task все прочитает.
- _rcwindkessel.h_ - класс для описания элементов Виндексселя.
- _heart_part.h heart_part.cpp_ - класс, отвечающий за описание и расчет модели сердца.
- _heart_advanced_valves.h_ - класс, отвечающий за модель сердца с клапанами.
- _heart_reg.h_ - класс, отвечающий за модель сердца с патологией в виде регургитации клапанов.
- _detailed_run.h_ - класс для запуска расчета с определенными параметрами и сохранением всех данных в файлы для дальнейшего анализа.
- _csv_reader.h_ - вспомогательный класс для работы с csv файлами.

**На этом все!**
Для вопросов tg: @rogovrt
