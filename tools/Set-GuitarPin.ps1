[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$Pin,

    [Parameter(Mandatory = $false)]
    [string]$EnvPath,

    [Parameter(Mandatory = $false)]
    [string]$Uri = "https://mqttguitarchordwebsite.ayatosync13.workers.dev/api/admin/pin",

    [Parameter(Mandatory = $false)]
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

if ($PSVersionTable.PSEdition -eq "Desktop") {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
}

function Get-DotEnvValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Env file was not found: $Path"
    }

    foreach ($line in Get-Content -LiteralPath $Path) {
        $trimmed = $line.Trim()
        if (-not $trimmed -or $trimmed.StartsWith("#")) {
            continue
        }

        $parts = $trimmed -split "=", 2
        if ($parts.Count -ne 2 -or $parts[0].Trim() -ne $Name) {
            continue
        }

        $value = $parts[1].Trim()
        if ($value.Length -ge 2) {
            $first = $value.Substring(0, 1)
            $last = $value.Substring($value.Length - 1, 1)
            if (($first -eq '"' -and $last -eq '"') -or ($first -eq "'" -and $last -eq "'")) {
                $value = $value.Substring(1, $value.Length - 2)
            }
        }
        return $value
    }

    return $null
}

function Test-PinFormat {
    param(
        [Parameter(Mandatory = $false)]
        [AllowEmptyString()]
        [string]$Value
    )

    return $Value -match "^[0-9]{4}$"
}

if ($SelfTest) {
    $tempEnvPath = [System.IO.Path]::GetTempFileName()
    try {
        Set-Content -LiteralPath $tempEnvPath -Value @(
            "# test fixture only"
            "STAFF_SECRET=dummy-test-value"
        ) -Encoding ASCII

        if ((Get-DotEnvValue -Path $tempEnvPath -Name "STAFF_SECRET") -ne "dummy-test-value") {
            throw "Self-test failed: STAFF_SECRET parsing"
        }
        if (-not (Test-PinFormat -Value "1234")) {
            throw "Self-test failed: valid PIN was rejected"
        }
        foreach ($invalidPin in @("123", "12345", "12a4", "")) {
            if (Test-PinFormat -Value $invalidPin) {
                throw "Self-test failed: invalid PIN was accepted: $invalidPin"
            }
        }
        Write-Host "Self-test passed."
        return
    }
    finally {
        Remove-Item -LiteralPath $tempEnvPath -Force -ErrorAction SilentlyContinue
    }
}

if ([string]::IsNullOrWhiteSpace($EnvPath)) {
    $sameFolderEnv = Join-Path $PSScriptRoot ".env"
    $parentFolderEnv = Join-Path (Split-Path $PSScriptRoot -Parent) ".env"

    if (Test-Path -LiteralPath $sameFolderEnv -PathType Leaf) {
        $EnvPath = $sameFolderEnv
    }
    elseif (Test-Path -LiteralPath $parentFolderEnv -PathType Leaf) {
        $EnvPath = $parentFolderEnv
    }
    elseif ((Split-Path $PSScriptRoot -Leaf) -eq "tools") {
        $EnvPath = $parentFolderEnv
    }
    else {
        $EnvPath = $sameFolderEnv
    }
}

$staffSecret = Get-DotEnvValue -Path $EnvPath -Name "STAFF_SECRET"
if ([string]::IsNullOrWhiteSpace($staffSecret)) {
    throw "STAFF_SECRET is missing or empty in: $EnvPath"
}

if ([string]::IsNullOrWhiteSpace($Pin)) {
    $Pin = Read-Host "Enter the 4-digit PIN"
}

if (-not (Test-PinFormat -Value $Pin)) {
    throw "PIN must be exactly 4 digits."
}

$headers = @{
    Authorization = "Bearer $staffSecret"
}
$body = @{ pin = $Pin } | ConvertTo-Json -Compress

try {
    $result = Invoke-RestMethod `
        -Method Post `
        -Uri $Uri `
        -Headers $headers `
        -ContentType "application/json" `
        -Body $body

    if ($result.ok -eq $true) {
        Write-Host "PIN was set successfully. JST date: $($result.setOn)"
    }
    else {
        throw "The API returned an unexpected response: $($result | ConvertTo-Json -Compress)"
    }
}
catch {
    $detail = $_.ErrorDetails.Message
    if ([string]::IsNullOrWhiteSpace($detail)) {
        $detail = $_.Exception.Message
    }
    throw "PIN setting failed: $detail"
}
finally {
    $staffSecret = $null
    $headers = $null
    $body = $null
}
