module LaunchSIP
    SETTINGS = {}
    File.open("settings.yml", "r") do |file|
        YAML.load(file.read()).each_pair do |key, value|
            SETTINGS[key] = value
        end
    end
end
