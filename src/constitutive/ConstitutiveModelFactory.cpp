
#include "ConstitutiveModelFactory.hpp"

#include "mechanical/solid/J2Plasticity.hpp"
#include "mechanical/solid/J2PlasticityDamage.hpp"

#include "mechanical/solid/FiniteJ2Plasticity.hpp"

#include "mechanical/solid/AffineMicrosphere.hpp"
#include "mechanical/solid/NonAffineMicrosphere.hpp"

#include "mechanical/solid/NeoHooke.hpp"

#include "thermal/IsotropicDiffusion.hpp"

#include "mechanical/plane/IsotropicLinearElasticity.hpp"

#include <json/value.h>
#include <stdexcept>

namespace neon::mechanical::solid
{
std::unique_ptr<ConstitutiveModel> make_constitutive_model(std::shared_ptr<InternalVariables>& variables,
                                                           Json::Value const& material_data,
                                                           Json::Value const& mesh_data)
{
    if (!mesh_data.isMember("ConstitutiveModel"))
    {
        throw std::runtime_error("Missing \"ConstitutiveModel\" in \"Mesh\"");
    }

    auto const& constitutive_model = mesh_data["ConstitutiveModel"];

    if (!constitutive_model.isMember("Name"))
    {
        throw std::runtime_error("Missing \"Name\" in \"ConstitutiveModel\"");
    }

    auto const& model_name = constitutive_model["Name"].asString();

    if (model_name == "NeoHooke")
    {
        return std::make_unique<NeoHooke>(variables, material_data);
    }
    else if (model_name == "Microsphere")
    {
        if (!constitutive_model.isMember("Type"))
        {
            throw std::runtime_error("Missing \"Type\" as \"Affine\" or \"NonAffine\" in "
                                     "Microsphere model");
        }

        if (!constitutive_model.isMember("Quadrature"))
        {
            throw std::runtime_error("Missing \"Quadrature\" as \"BO21\", \"BO33\" or \"FM900\" in "
                                     "Microsphere model");
        }

        auto const& model_type = constitutive_model["Type"].asString();

        // TODO Check for ageing model

        std::map<std::string, UnitSphereQuadrature::Rule> const str_to_enum =
            {{"BO21", UnitSphereQuadrature::Rule::BO21},
             {"BO33", UnitSphereQuadrature::Rule::BO33},
             {"FM900", UnitSphereQuadrature::Rule::FM900}};

        auto const entry = str_to_enum.find(constitutive_model["Quadrature"].asString());

        if (entry == str_to_enum.end())
        {
            throw std::runtime_error("\"Quadrature\" field must be \"BO21\", \"BO33\" or "
                                     "\"FM900\"");
        }

        if (model_type == "Affine")
        {
            return std::make_unique<AffineMicrosphere>(variables, material_data, entry->second);
        }
        else if (model_type == "NonAffine")
        {
            return std::make_unique<NonAffineMicrosphere>(variables, material_data, entry->second);
        }
        else
        {
            throw std::runtime_error("Microsphere model options are \"Affine\" or \"Nonaffine\"");
        }
    }
    else if (model_name == "IsotropicLinearElasticity")
    {
        return std::make_unique<IsotropicLinearElasticity>(variables, material_data);
    }
    else if (model_name == "J2Plasticity")
    {
        if (!constitutive_model.isMember("FiniteStrain"))
        {
            throw std::runtime_error("\"J2Plasticity\" must have a boolean value for "
                                     "\"FiniteStrain\"");
        }

        if (mesh_data["ConstitutiveModel"]["FiniteStrain"].asBool())
        {
            return std::make_unique<FiniteJ2Plasticity>(variables, material_data);
        }
        return std::make_unique<J2Plasticity>(variables, material_data);
    }
    else if (model_name == "ChabocheDamage")
    {
        if (mesh_data["ConstitutiveModel"]["FiniteStrain"].asBool())
        {
            throw std::runtime_error("\"ChabocheDamage\" is not implemented for "
                                     "\"FiniteStrain\"");
        }
        return std::make_unique<J2PlasticityDamage>(variables, material_data);
    }
    throw std::runtime_error("The model name " + model_name + " is not recognised\n"
                             + "Supported models are \"NeoHooke\", \"Microsphere\" "
                               "and \"J2Plasticity\"\n");
    return nullptr;
}
}

namespace neon::mechanical::plane
{
std::unique_ptr<ConstitutiveModel> make_constitutive_model(std::shared_ptr<InternalVariables>& variables,
                                                           Json::Value const& material_data,
                                                           Json::Value const& mesh_data)
{
    if (!mesh_data.isMember("ConstitutiveModel"))
    {
        throw std::runtime_error("Missing \"ConstitutiveModel\" in \"Mesh\"");
    }

    auto const& constitutive_model = mesh_data["ConstitutiveModel"];

    if (!constitutive_model.isMember("Name"))
    {
        throw std::runtime_error("Missing \"Name\" in \"ConstitutiveModel\"");
    }

    auto const& model_name = constitutive_model["Name"].asString();

    if (model_name == "PlaneStrain")
    {
        return std::make_unique<IsotropicLinearElasticity>(variables,
                                                           material_data,
                                                           IsotropicLinearElasticity::plane::strain);
    }
    else if (model_name == "PlaneStress")
    {
        return std::make_unique<IsotropicLinearElasticity>(variables,
                                                           material_data,
                                                           IsotropicLinearElasticity::plane::stress);
    }
    throw std::runtime_error("The model name " + model_name + " is not recognised\n"
                             + "Supported models are \"PlaneStrain\" and \"PlaneStress\"\n");
    return nullptr;
}
}

namespace neon::diffusion
{
std::unique_ptr<ConstitutiveModel> make_constitutive_model(std::shared_ptr<InternalVariables>& variables,
                                                           Json::Value const& material_data,
                                                           Json::Value const& mesh_data)
{
    if (!mesh_data.isMember("ConstitutiveModel"))
    {
        throw std::runtime_error("A \"ConstitutiveModel\" was not specified in \"Mesh\"");
    }

    auto const& constitutive_model = mesh_data["ConstitutiveModel"];

    if (!constitutive_model.isMember("Name"))
    {
        throw std::runtime_error("\"ConstitutiveModel\" requires a \"Name\" field");
    }

    if (constitutive_model["Name"].asString() == "IsotropicDiffusion")
    {
        return std::make_unique<IsotropicDiffusion>(variables, material_data);
    }

    throw std::runtime_error("The model name " + constitutive_model["Name"].asString()
                             + " is not recognised\n"
                             + "The supported model is \"IsotropicDiffusion\"\n");

    return nullptr;
}
}
