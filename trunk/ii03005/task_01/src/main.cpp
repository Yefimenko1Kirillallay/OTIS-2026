#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <memory>
#include <clocale>
#include <stdexcept>
#include <format>
#include <limits>

namespace fs = std::filesystem;

enum class InputType
{
    Step,
    Impulse,
    Harmonic
};

class Model
{
private:
    int n{};
    int t{};
    InputType inputType{InputType::Step};
    std::vector<double> y{};
    std::vector<double> u{};
    std::string filename{};

    static int validateStepCount(int stepCount)
    {
        if (stepCount < 1)
        {
            throw std::invalid_argument(
                "Number of steps must be greater than zero."
            );
        }

        return stepCount;
    }

protected:
    virtual double GetTime(int index) const
    {
        return static_cast<double>(index);
    }

    int currentStep() const
    {
        return t;
    }

    double outputAt(int index) const
    {
        return y.at(static_cast<std::size_t>(index));
    }

    void setOutput(int index, double value)
    {
        y.at(static_cast<std::size_t>(index)) = value;
    }

    double inputAt(int index) const
    {
        return u.at(static_cast<std::size_t>(index));
    }

    void advanceStep()
    {
        ++t;
    }

    void CountU(double inputValue = 0.0, double timeStep = 1.0)
    {
        switch (inputType)
        {
            using enum InputType;

        case Step:
            u.assign(static_cast<std::size_t>(n), inputValue);
            break;

        case Impulse:
            u.assign(static_cast<std::size_t>(n), 0.0);
            u[0] = 1.0;
            break;

        case Harmonic:
            u.resize(static_cast<std::size_t>(n));

            for (int index = 0; index < n; ++index)
            {
                u[static_cast<std::size_t>(index)] =
                    std::sin(static_cast<double>(index) * timeStep);
            }
            break;
        }
    }

public:
    Model(int stepCount, InputType selectedInputType)
        : n(validateStepCount(stepCount)),
          t(0),
          inputType(selectedInputType),
          y(static_cast<std::size_t>(n), 0.0),
          u(static_cast<std::size_t>(n), 0.0)
    {
    }

    virtual ~Model() = default;

    virtual void NextStep() = 0;

    std::string GetFilename() const
    {
        return filename;
    }

    void StartModel()
    {
        while (t < n - 1)
        {
            NextStep();
        }
    }

    void show_y() const
    {
        std::cout << '\n'
                  << "+------------+----------------+----------------+\n"
                  << "|      t     |        u       |        y       |\n"
                  << "+------------+----------------+----------------+\n";

        for (int index = 0; index < n; ++index)
        {
            std::cout << std::format(
                "| {:>10.4f} | {:>14.4f} | {:>14.4f} |\n",
                GetTime(index),
                inputAt(index),
                outputAt(index)
            );
        }

        std::cout << "+------------+----------------+----------------+\n\n";
    }

    void saveCSV(const std::string& userInput)
    {
        const fs::path inputPath(userInput);
        auto outputPath = inputPath.filename();

        if (outputPath.empty() ||
            outputPath == "." ||
            outputPath == "..")
        {
            std::cerr << "Invalid filename.\n";
            return;
        }

        if (outputPath.extension() != ".csv")
        {
            outputPath.replace_extension(".csv");
        }

        filename = outputPath.string();

        std::ofstream file(filename);

        if (!file.is_open())
        {
            std::cerr << "Не удалось открыть файл.\n";
            return;
        }

        file << "t;u;y\n";

        for (int index = 0; index < n; ++index)
        {
            file << std::format(
                "{:.6f};{:.6f};{:.6f}\n",
                GetTime(index),
                inputAt(index),
                outputAt(index)
            );
        }

        if (!file)
        {
            std::cerr << "Ошибка записи CSV-файла.\n";
            return;
        }

        std::cout << "CSV saved: " << filename << '\n';
    }
};

class Liner : public Model
{
private:
    double a1{};
    double a2{};
    double b{};
    int k{};

public:
    Liner(
        double initialA1,
        double initialA2,
        double initialB,
        int delay,
        int stepCount,
        InputType selectedInputType,
        double inputValue = 0.0
    )
        : Model(stepCount, selectedInputType),
          a1(initialA1),
          a2(initialA2),
          b(initialB),
          k(delay)
    {
        if (delay < 0)
        {
            throw std::invalid_argument(
                "Delay k cannot be negative."
            );
        }

        CountU(inputValue);
    }

    void NextStep() override
    {
        const auto step = currentStep();
        const auto currentY = outputAt(step);
        double delayedY{};

        if (step >= k)
        {
            delayedY = outputAt(step - k);
        }

        const auto newY =
            a1 * currentY +
            a2 * delayedY +
            b * inputAt(step);

        setOutput(step + 1, newY);
        advanceStep();
    }

};


class NonLiner: public Model
{
private:
    double a{};
    double b{};
    double e{};

public:
    NonLiner(
        double initialA,
        double initialB,
        double initialE,
        int stepCount,
        InputType selectedInputType,
        double inputValue = 0.0
    )
        : Model(stepCount, selectedInputType),
          a(initialA),
          b(initialB),
          e(initialE)
    {
        CountU(inputValue);
    }

    double relay(double inputValue, double outputValue) const
    {
        if (inputValue > e ||
            (inputValue >= -e && outputValue > 0.0))
        {
            return 1.0;
        }

        if (inputValue < -e ||
            (inputValue <= e && outputValue <= 0.0))
        {
            return -1.0;
        }

        return 0.0;
    }

    void NextStep() override
    {
        const auto step = currentStep();
        const auto currentY = outputAt(step);

        const auto newY =
            a * currentY +
            b * relay(inputAt(step), currentY);

        setOutput(step + 1, newY);
        advanceStep();
    }
};


class Differential : public Model
{
private:
    double b{};
    double dt{};

protected:
    double GetTime(int index) const override
    {
        return static_cast<double>(index) * dt;
    }

public:
    Differential(
        double initialB,
        double timeStep,
        int stepCount,
        InputType selectedInputType,
        double inputValue = 0.0
    )
        : Model(stepCount, selectedInputType),
          b(initialB),
          dt(timeStep)
    {
        if (timeStep <= 0.0)
        {
            throw std::invalid_argument(
                "Time step dt must be greater than zero."
            );
        }

        CountU(inputValue, dt);
    }

    void NextStep() override
    {
        const auto step = currentStep();
        const auto currentY = outputAt(step);

        const auto newY =
            currentY + dt * b * std::tanh(inputAt(step));

        setOutput(step + 1, newY);
        advanceStep();
    }
};

void printInputMenu()
{
    std::cout
        << "\n========== INPUT TYPE ==========\n"
        << "1. Step\n"
        << "2. Impulse\n"
        << "3. Harmonic\n"
        << "Choose input: ";
}

InputType chooseInputType()
{
    while (true)
    {
        printInputMenu();

        if (int choice{}; std::cin >> choice)
        {
            switch (choice)
            {
                using enum InputType;

            case 1:
                return Step;

            case 2:
                return Impulse;

            case 3:
                return Harmonic;

            default:
                std::cout << "Invalid choice. Try again.\n";
                break;
            }
        }
        else
        {
            throw std::invalid_argument("Invalid input type.");
        }
    }
}


void showGraph(const std::string& filename)
{
    const auto command =
        std::string(PYTHON_EXECUTABLE) +
        " \"" + std::string(PLOT_SCRIPT) +
        "\" \"" + filename + "\"";

    if (const auto result = std::system(command.c_str());
        result != 0)
    {
        std::cerr << "Не удалось запустить Python.\n";
    }
}

std::unique_ptr<Model> createLinearModel(
    int stepCount,
    InputType inputType
)
{
    double a1{};
    double a2{};
    double b{};
    double inputValue{};
    int delay{};

    std::cout << "Enter a1: ";
    if (!(std::cin >> a1))
    {
        throw std::invalid_argument("Invalid a1.");
    }

    std::cout << "Enter a2: ";
    if (!(std::cin >> a2))
    {
        throw std::invalid_argument("Invalid a2.");
    }

    std::cout << "Enter b: ";
    if (!(std::cin >> b))
    {
        throw std::invalid_argument("Invalid b.");
    }

    std::cout << "Enter delay k: ";
    if (!(std::cin >> delay))
    {
        throw std::invalid_argument("Invalid delay.");
    }

    std::cout << "Enter input value: ";
    if (!(std::cin >> inputValue))
    {
        throw std::invalid_argument("Invalid input value.");
    }

    return std::make_unique<Liner>(
        a1, a2, b, delay, stepCount, inputType, inputValue
    );
}

std::unique_ptr<Model> createNonlinearModel(
    int stepCount,
    InputType inputType
)
{
    double a{};
    double b{};
    double threshold{};
    double inputValue{};

    std::cout << "Enter a: ";
    if (!(std::cin >> a))
    {
        throw std::invalid_argument("Invalid a.");
    }

    std::cout << "Enter b: ";
    if (!(std::cin >> b))
    {
        throw std::invalid_argument("Invalid b.");
    }

    std::cout << "Enter threshold e: ";
    if (!(std::cin >> threshold))
    {
        throw std::invalid_argument("Invalid threshold.");
    }

    std::cout << "Enter input value: ";
    if (!(std::cin >> inputValue))
    {
        throw std::invalid_argument("Invalid input value.");
    }

    return std::make_unique<NonLiner>(
        a, b, threshold, stepCount, inputType, inputValue
    );
}

std::unique_ptr<Model> createDifferentialModel(
    int stepCount,
    InputType inputType
)
{
    double b{};
    double timeStep{};
    double inputValue{};

    std::cout << "Enter b: ";
    if (!(std::cin >> b))
    {
        throw std::invalid_argument("Invalid b.");
    }

    std::cout << "Enter time step dt: ";
    if (!(std::cin >> timeStep))
    {
        throw std::invalid_argument("Invalid dt.");
    }

    std::cout << "Enter input value: ";
    if (!(std::cin >> inputValue))
    {
        throw std::invalid_argument("Invalid input value.");
    }

    return std::make_unique<Differential>(
        b, timeStep, stepCount, inputType, inputValue
    );
}

std::unique_ptr<Model> createModel(
    int modelChoice,
    int stepCount,
    InputType inputType
)
{
    switch (modelChoice)
    {
    case 1:
        return createLinearModel(stepCount, inputType);

    case 2:
        return createNonlinearModel(stepCount, inputType);

    case 3:
        return createDifferentialModel(stepCount, inputType);

    default:
        return nullptr;
    }
}

void saveModelCSV(Model& model)
{
    std::string filename{};

    std::cout << "Enter CSV filename: ";

    if (!(std::cin >> filename))
    {
        throw std::invalid_argument("Invalid filename.");
    }

    model.saveCSV(filename);
}

void showModelGraph(const Model& model)
{
    const auto filename = model.GetFilename();

    if (filename.empty())
    {
        std::cout << "Сначала сохраните модель в CSV.\n";
        return;
    }

    showGraph(filename);
}

void printModelMenu()
{
    std::cout
        << "\n========== MODEL MENU ==========\n"
        << "1. Run model\n"
        << "2. Show results\n"
        << "3. Save CSV\n"
        << "4. Show graph\n"
        << "0. Return\n"
        << "Choose: ";
}

void runModelAction(Model& model)
{
    model.StartModel();
    std::cout << "Model calculated.\n";
}

void handleModelChoice(int choice, Model& model, bool& running)
{
    switch (choice)
    {
    case 1:
        runModelAction(model);
        break;

    case 2:
        model.show_y();
        break;

    case 3:
        saveModelCSV(model);
        break;

    case 4:
        showModelGraph(model);
        break;

    case 0:
        running = false;
        break;

    default:
        std::cout << "Invalid choice. Try again.\n";
        break;
    }
}

void runModelMenu(Model& model)
{
    bool running = true;

    while (running)
    {
        printModelMenu();

        if (int choice{}; std::cin >> choice)
        {
            handleModelChoice(choice, model, running);
        }
        else
        {
            throw std::invalid_argument("Invalid menu choice.");
        }
    }
}

void printMainMenu()
{
    std::cout
        << "\n========== MAIN MENU ==========\n"
        << "1. Linear model\n"
        << "2. Nonlinear model\n"
        << "3. Differential model\n"
        << "0. Exit\n"
        << "Choose model: ";
}

void runSelectedModel(int modelChoice)
{
    int stepCount{};

    std::cout << "Enter number of steps: ";

    if (!(std::cin >> stepCount))
    {
        throw std::invalid_argument("Invalid step count.");
    }

    if (stepCount < 1)
    {
        throw std::invalid_argument(
            "Number of steps must be greater than zero."
        );
    }

    const auto inputType = chooseInputType();
    auto model = createModel(modelChoice, stepCount, inputType);

    if (model != nullptr)
    {
        runModelMenu(*model);
    }
}

int main()
{
    std::setlocale(LC_ALL, "");

    try
    {
        bool running = true;

        while (running)
        {
            printMainMenu();

            if (int choice{}; std::cin >> choice)
            {
                if (choice == 0)
                {
                    running = false;
                }
                else if (choice >= 1 && choice <= 3)
                {
                    runSelectedModel(choice);
                }
                else
                {
                    std::cout << "Invalid model choice.\n";
                }
            }
            else
            {
                throw std::invalid_argument("Invalid model choice.");
            }
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << "Error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
