# ============================================================================
# Compilation et installation du pilote Qt QMYSQL (Windows / MinGW / Qt 6)
# ----------------------------------------------------------------------------
# Qt ne fournit pas le pilote QMYSQL dans les paquets binaires. Ce script :
#   1. télécharge les sources du plugin MySQL correspondant à la version de Qt ;
#   2. télécharge MySQL Connector/C (en-têtes + libmysql.dll) ;
#   3. compile qsqlmysql.dll avec MinGW ;
#   4. l'installe dans <Qt>/plugins/sqldrivers et copie libmysql.dll dans <Qt>/bin.
#
# Prérequis : Qt (MinGW), MinGW, connexion Internet, PowerShell.
# Exemple :
#   powershell -ExecutionPolicy Bypass -File scripts\build_qmysql_plugin.ps1 `
#       -QtDir "C:\Qt\6.5.3\mingw_64" -MingwBin "C:\Qt\Tools\mingw1120_64\bin"
# ============================================================================

param(
    [string]$QtDir = "C:\Qt\6.5.3\mingw_64",
    [string]$MingwBin = "C:\Qt\Tools\mingw1120_64\bin",
    [string]$QtVersion = "6.5.3",
    [string]$ConnectorUrl = "https://dev.mysql.com/get/Downloads/Connector-C/mysql-connector-c-6.1.11-winx64.zip"
)

$ErrorActionPreference = "Stop"

function Write-Etape([string]$texte) {
    Write-Host ""
    Write-Host "==> $texte" -ForegroundColor Cyan
}

$work = Join-Path $env:TEMP "qmysql_plugin_build"
New-Item -ItemType Directory -Force -Path $work | Out-Null
$env:Path = "$QtDir\bin;$MingwBin;$env:Path"

$moc = Join-Path $QtDir "bin\moc.exe"
$gpp = Join-Path $MingwBin "g++.exe"
if (-not (Test-Path $moc)) { throw "moc.exe introuvable : $moc" }
if (-not (Test-Path $gpp)) { throw "g++.exe introuvable : $gpp" }

Write-Etape "Téléchargement des sources du plugin ($QtVersion)"
$base = "https://code.qt.io/cgit/qt/qtbase.git/plain/src/plugins/sqldrivers/mysql"
$fichiers = @{
    "qsql_mysql.cpp"   = "$base/qsql_mysql.cpp?h=$QtVersion"
    "qsql_mysql_p.h"   = "$base/qsql_mysql_p.h?h=$QtVersion"
    "main.cpp"         = "$base/main.cpp?h=$QtVersion"
    "mysql.json"       = "$base/mysql.json?h=$QtVersion"
}
foreach ($nom in $fichiers.Keys) {
    $destination = Join-Path $work $nom
    if (-not (Test-Path $destination)) {
        curl.exe -L --fail --max-time 120 -o $destination $fichiers[$nom]
        if ($LASTEXITCODE -ne 0) { throw "Téléchargement impossible : $nom" }
    }
}

Write-Etape "Téléchargement de MySQL Connector/C"
$connectorZip = Join-Path $work "mysql-connector-c.zip"
$connectorDir = Join-Path $work "mysql-connector-c"
if (-not (Test-Path $connectorDir)) {
    if (-not (Test-Path $connectorZip)) {
        curl.exe -L --fail --max-time 300 -o $connectorZip $ConnectorUrl
        if ($LASTEXITCODE -ne 0) { throw "Téléchargement de Connector/C impossible." }
    }
    Expand-Archive -Path $connectorZip -DestinationPath $connectorDir -Force
}
$connectorRoot = Get-ChildItem $connectorDir -Directory | Select-Object -First 1
if (-not $connectorRoot) { throw "Archive Connector/C invalide." }
$mysqlInclude = Join-Path $connectorRoot.FullName "include"
$mysqlLib = Join-Path $connectorRoot.FullName "lib"
$libmysql = Join-Path $mysqlLib "libmysql.dll"
if (-not (Test-Path $libmysql)) { throw "libmysql.dll introuvable dans Connector/C." }

Write-Etape "Génération des fichiers moc"
Push-Location $work
& $moc (Join-Path $work "qsql_mysql_p.h") -o (Join-Path $work "moc_qsql_mysql_p.cpp")
& $moc (Join-Path $work "main.cpp") -o (Join-Path $work "main.moc")

Write-Etape "Compilation de qsqlmysql.dll"
$q = $QtDir.Replace("\", "/")
$my = $connectorRoot.FullName.Replace("\", "/")
$arguments = @(
    "-shared", "-o", "qsqlmysql.dll",
    "qsql_mysql.cpp", "main.cpp",
    "-DQT_PLUGIN", "-DQT_SQL_LIB", "-DQT_CORE_LIB",
    "-DUNICODE", "-D_UNICODE", "-DWIN32", "-DMINGW_HAS_SECURE_API=1", "-DQT_NO_DEBUG",
    "-I.", "-I$my/include",
    "-I$q/include", "-I$q/include/QtCore", "-I$q/include/QtSql",
    "-I$q/include/QtCore/6.5.3", "-I$q/include/QtCore/6.5.3/QtCore",
    "-I$q/include/QtSql/6.5.3", "-I$q/include/QtSql/6.5.3/QtSql",
    "-I$q/include/QtSql/6.5.3/QtSql/private",
    "-I$q/include/QtCore/6.5.3/QtCore/private",
    "-I$q/mkspecs/win32-g++",
    "-L$q/lib", "-lQt6Sql", "-lQt6Core",
    "-L$my/lib", "-lmysql"
)
& $gpp @arguments
if ($LASTEXITCODE -ne 0) { throw "La compilation du plugin a échoué." }
Pop-Location

Write-Etape "Installation du pilote"
$pluginTarget = Join-Path $QtDir "plugins\sqldrivers"
New-Item -ItemType Directory -Force -Path $pluginTarget | Out-Null
Copy-Item (Join-Path $work "qsqlmysql.dll") (Join-Path $pluginTarget "qsqlmysql.dll") -Force
Copy-Item $libmysql (Join-Path $QtDir "bin\libmysql.dll") -Force

Write-Etape "Terminé"
Write-Host "Plugin installé : $pluginTarget\qsqlmysql.dll" -ForegroundColor Green
Write-Host "Bibliothèque client : $QtDir\bin\libmysql.dll" -ForegroundColor Green
Write-Host "Redémarrez Qt Creator puis vérifiez QSqlDatabase::drivers()."
