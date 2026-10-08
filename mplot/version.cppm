module;

#include <string>
#include <cstdint>

export module mplot.version;

/*!
 * This header defines a simple major/minor version for mathplot.
 *
 * \author Seb James
 * \date April 2024
 */

export namespace mplot
{
    //! A version definition for the whole of mathplot
    constexpr std::uint32_t version_major = 2;
    constexpr std::uint32_t version_minor = 1;
    //! Returns a string for the version of the mathplot library
    std::string version_string()
    {
        std::string vers = std::to_string (mplot::version_major) + std::string(".") + std::to_string (mplot::version_minor);
        return vers;
    }

} // namespace mplot
