cmake_minimum_required(VERSION 3.16)

# La version, en un seul endroit et sous la forme la plus simple qui soit :
# trois nombres dans un fichier, que la publication avance et que rien n'a
# besoin d'aller extraire du code.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" BELOTESOL_VERSION)
string(STRIP "${BELOTESOL_VERSION}" BELOTESOL_VERSION)

project(belotesol VERSION ${BELOTESOL_VERSION} LANGUAGES CXX)

# Pour qu'avancer la version suffise : le build suivant reconfigure plutôt que
# de porter le numéro d'hier.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

include(GNUInstallDirs)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets)

# La cartothèque : le format des jeux de cartes, la liste de ceux qui sont
# installés, et le style que les jeux de la maison recopiaient chacun de son
# côté. Bibliothèque statique : elle entre dans le binaire, et belotesol reste
# un exécutable et rien d'autre.
add_subdirectory(../cartes-set-creator/lib cartotheque)

# Quel build c'est. Écrit à chaque build plutôt qu'à chaque configure, pour que
# la version sur la ligne de commande et sur le paquet soit le build qui est
# devant vous ; voir cmake/GenerateVersion.cmake.
set(BUILD_VERSION_CPP ${CMAKE_CURRENT_BINARY_DIR}/BuildVersion.cpp)
set(BUILD_VERSION_TXT ${CMAKE_CURRENT_BINARY_DIR}/BuildVersion.txt)
set(GENERATE_VERSION ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateVersion.cmake)

# Une fois maintenant, pour que le fichier existe avant que rien soit compilé.
execute_process(COMMAND ${CMAKE_COMMAND} -DVERSION=${BELOTESOL_VERSION}
    -DOUT_CPP=${BUILD_VERSION_CPP} -DOUT_TXT=${BUILD_VERSION_TXT} -P ${GENERATE_VERSION})

add_custom_target(build-version ALL
    BYPRODUCTS ${BUILD_VERSION_CPP} ${BUILD_VERSION_TXT}
    COMMAND ${CMAKE_COMMAND} -DVERSION=${BELOTESOL_VERSION}
        -DOUT_CPP=${BUILD_VERSION_CPP} -DOUT_TXT=${BUILD_VERSION_TXT} -P ${GENERATE_VERSION}
    COMMENT "Stamping the build with the time it was made")
set_source_files_properties(${BUILD_VERSION_CPP} PROPERTIES GENERATED TRUE)

add_executable(belotesol
    ${BUILD_VERSION_CPP}
    src/Version.h
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    # Le look : la couleur du drap, la plaque du profil et l'habillage des
    # menus. Palette et DialogChrome sont dans la cartothèque.
    src/TileButton.cpp
    src/TileButton.h
    src/ProfileChip.cpp
    src/ProfileChip.h
    src/MenuStyle.cpp
    src/MenuStyle.h
    # Les cartes elles-mêmes : ce qu'elles sont, ce qu'elles valent -- deux
    # barèmes, selon l'atout -- et comment elles sont dessinées. Rien en
    # dessous de cette ligne ne connaît les règles.
    src/Cards.cpp
    src/Cards.h
    src/FeltArt.cpp
    src/FeltArt.h
    # Les règles : l'enchère, les annonces, les huit plis, et la marque. Rien
    # ici ne sait qu'il y a un écran.
    src/Tables.cpp
    src/Tables.h
    src/TableLook.cpp
    src/TableLook.h
    src/Annonces.cpp
    src/Annonces.h
    src/Belote.cpp
    src/Belote.h
    # Les six autour de la table : qui ils sont, ce qu'ils ont appris, et
    # comment ils décident.
    src/Rivals.cpp
    src/Rivals.h
    src/Profiles.cpp
    src/Profiles.h
    src/Brain.cpp
    src/Brain.h
    # La table à l'écran, et la page qui y fait tourner une partie.
    src/TableView.cpp
    src/TableView.h
    src/TablePage.cpp
    src/TablePage.h
    src/ScoreBoard.cpp
    src/ScoreBoard.h
    src/VictoryEffect.cpp
    src/VictoryEffect.h
    # Le menu d'ouverture et les deux boîtes de dialogue.
    src/RivalCard.cpp
    src/RivalCard.h
    src/TablePicker.cpp
    src/TablePicker.h
    src/SettingsDialog.cpp
    src/SettingsDialog.h
    src/NameDialog.cpp
    src/NameDialog.h
    src/HelpDialog.cpp
    src/HelpDialog.h
)

# Pour que le tampon soit écrit avant que les sources qui le portent soient
# compilées.
add_dependencies(belotesol build-version)

target_link_libraries(belotesol PRIVATE cartotheque Qt6::Widgets)

install(TARGETS belotesol RUNTIME DESTINATION bin)

# Pour qu'un shell de bureau puisse dire à quelle application une fenêtre
# appartient. Le nom du fichier doit être celui de l'app_id que l'application
# pose avec setDesktopFileName().
install(FILES data/belotesol.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)

# Le paquet Debian : `cmake --build build --target deb`.
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "belotesol")
set(CPACK_PACKAGE_VENDOR "belotesol")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "La belote sur tapis vert, à quatre autour de la table")
# Une seule chaîne, et non trois : une liste arriverait dans le fichier control
# avec les points-virgules qui la joignent encore en place.
set(CPACK_PACKAGE_DESCRIPTION
    "La belote française, jouée à quatre contre trois adversaires qui apprennent
d'une partie à l'autre : chacun garde son expérience, débutant, intermédiaire ou
expert, le premier invité s'assoit en face de vous, et trois profils séparent
votre table de celle des autres.")
set(CPACK_PACKAGE_CONTACT "pdulvp <pdulvp@laposte.net>")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "games")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
# Les bibliothèques Qt contre lesquelles il a été construit, déduites du
# binaire lui-même plutôt qu'écrites et laissées à vieillir.
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
# Les symboles de débogage sont pour l'arbre de build, pas pour le disque de
# tout le monde.
set(CPACK_STRIP_FILES TRUE)
# Les icônes que portent les boutons du haut viennent du thème d'icônes du
# bureau, et un thème actuel les livre en SVG : sans le greffon qui en lit un,
# elles sont nues. Pas une dépendance -- chaque bouton porte aussi son mot --
# mais presque.
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "qt6-svg-plugins")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_BUILD_VERSION_FILE "${BUILD_VERSION_TXT}")
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPackOptions.cmake")
# La version est celle du projet ; le tampon du build part comme release
# Debian, pour que deux paquets de 1.0.0 construits des jours différents soient
# distingués et que le plus tardif soit le plus récent.
set(CPACK_PACKAGE_VERSION "${BELOTESOL_VERSION}")
include(CPack)

# Un seul pas d'un arbre de travail à un paquet.
add_custom_target(deb
    COMMAND ${CMAKE_COMMAND} -E env BELOTESOL_KEEP_VERSION=1
        ${CMAKE_CPACK_COMMAND} -G DEB --config ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake
    DEPENDS belotesol
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building the Debian package")
