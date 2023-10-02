// Fill out your copyright notice in the Description page of Project Settings.

#include "SubmarineGameInstance.h"

#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlinesubsystemSessionSettings.h"
#include "SubmarineSession.h"
#include "Kismet/GameplayStatics.h"

USubmarineGameInstance::USubmarineGameInstance()
{
	bIsLan = false;
	NumPlayers = 12;
	bUsePresence = true;
}

void USubmarineGameInstance::Init()
{
	Super::Init();

	OnlineSubsystem = IOnlineSubsystem::Get();
	
}

void USubmarineGameInstance::LogNoSubsystem()
{
	UE_LOG(LogTemp, Error, TEXT("Online Subsystem not valid"))
}

void USubmarineGameInstance::LogNoSessionInterface()
{
	UE_LOG(LogTemp, Error, TEXT("Failed to get Session interface."))
}

void USubmarineGameInstance::LogNoIdentityInterface()
{
	UE_LOG(LogTemp, Error, TEXT("Failed to get Identity interface."))
}

bool USubmarineGameInstance::IsLoggedIn()
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			return Identity->GetLoginStatus(0) == ELoginStatus::LoggedIn;
		}
		else
		{
			LogNoIdentityInterface();
			return false;
		}
	}
	else
	{
		LogNoSubsystem();
		return false;
	}
}



void USubmarineGameInstance::LogIn(const int PlayerNumber)
{
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			FOnlineAccountCredentials Credentials;
			Credentials.Id = FString();
			// const auto Guid = GetUniqueID();
			// Credentials.Id.AppendInt(Guid);
			Credentials.Token = FString();
			Credentials.Type = FString(LogInType);

			Identity->OnLoginCompleteDelegates->AddUObject(this, &USubmarineGameInstance::OnLoginComplete);
			Identity->Login(PlayerNumber, Credentials);
		}
		else
		{
			LogNoIdentityInterface();
		}
	}
	else
	{
		LogNoSubsystem();
	}
}

void USubmarineGameInstance::OnLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId,
	const FString& ErrorMessage)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Logging in successful!"))
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green,
				TEXT("Logged in Successfully!"));	
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Log In Failed: %s"), *ErrorMessage);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
				TEXT("Log in Failed... you might be able to try again, though."));	
		}
	}
	
	if (OnlineSubsystem)
	{
		if (const IOnlineIdentityPtr Identity = OnlineSubsystem->GetIdentityInterface())
		{
			Identity->ClearOnLoginCompleteDelegates(LocalUserNum, this);
		}
		else
		{
			LogNoIdentityInterface();
		}
	}
	else
	{
		LogNoSubsystem();
	}

	LogInCompleted.Broadcast(bWasSuccessful);
}

void USubmarineGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	bIsSearching = false;
	if (GEngine)
	{
		if (bWasSuccessful && SessionSearch->SearchResults.Num() == 0)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
				TEXT("Failed to find any open Sessions, but you're safe try to Join again."));
		}
		else if (!bWasSuccessful)
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
				TEXT("FindSessions failed. This might mean the build is broken..."));
		}
	}
	
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Success! Found %d sessions!"), SessionSearch->SearchResults.Num())
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Session search failed."))
	}
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface())
		{
			Session->ClearOnFindSessionsCompleteDelegates(this);
		}
	}
	FindSessionsCompleted.Broadcast(bWasSuccessful);
}

void USubmarineGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (OnlineSubsystem)
	{
		if (const auto Session = OnlineSubsystem->GetSessionInterface())
		{
			FString ConnectionInfo;
			Session->GetResolvedConnectString(SessionName, ConnectionInfo);
			if (ConnectionInfo.IsEmpty())
			{
				UE_LOG(LogTemp, Error, TEXT("ConnectionInfo was empty."))
				return;
			}
			if (APlayerController* Player = UGameplayStatics::GetPlayerController(GetWorld(), 0))
			{
				Player->ClientTravel(ConnectionInfo, TRAVEL_Absolute);
			}
		}
	}
}

int USubmarineGameInstance::GetNumSessionsFound()
{
	if (SessionSearch->SearchState != EOnlineAsyncTaskState::Done)
	{
		return 0;
	}
	FOnlineSessionSearchResult Result;
	return SessionSearch->SearchResults.Num();
}


void USubmarineGameInstance::CreateSession(const FString& SessionName)
{
	if (!IsLoggedIn())
	{
		UE_LOG(LogTemp, Error, TEXT("Can't create Session with Presence without being logged in."))
		return;
	}
	
	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface())
		{
			auto Settings = FOnlineSessionSettings();
			Settings.bIsDedicated = false;
			Settings.bShouldAdvertise = bAdvertise;
			Settings.bIsLANMatch = bIsLan;
			Settings.NumPublicConnections = NumPlayers;
			Settings.bAllowJoinInProgress = true;
			Settings.bAllowJoinViaPresence = bUsePresence;
			Settings.bUsesPresence = bUsePresence;
			Settings.bUseLobbiesVoiceChatIfAvailable = false;
			Settings.bUseLobbiesIfAvailable = bUseLobbies;

			// Settings.Set(SEARCH_KEYWORDS, SearchKeyword, EOnlineDataAdvertisementType::ViaOnlineService);
			const FName ResolvedName = SessionName.Equals("")
				? FName("SubmarineGame") : FName("Submarine" + SessionName);
			Session->OnCreateSessionCompleteDelegates.AddUObject(this, &USubmarineGameInstance::OnCreateSessionComplete);
			Session->CreateSession(0, ResolvedName, Settings);
		}
		else
		{
			LogNoSessionInterface();
		}
	}
	else
	{
		LogNoSubsystem();
	}
	
}

void USubmarineGameInstance::FindSessions()
{
	if (!IsLoggedIn())
	{
		UE_LOG(LogTemp, Error, TEXT("Must log in first!"))
		return;
	}

	if (OnlineSubsystem)
	{
		if (IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface())
		{
			SessionSearch = MakeShareable<FOnlineSessionSearch>(new FOnlineSessionSearch());
			//SessionSearch->QuerySettings.Set(SEARCH_KEYWORDS, SearchKeyword, EOnlineComparisonOp::Equals);
			// if (bUseLobbies)
			// {
			// 	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
			// }
			SessionSearch->MaxSearchResults = 10000;
			SessionSearch->bIsLanQuery = bIsLan;
			SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, bUseLobbies, EOnlineComparisonOp::Equals);
			SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, bUsePresence, EOnlineComparisonOp::Equals);
			Session->OnFindSessionsCompleteDelegates.AddUObject(this, &USubmarineGameInstance::OnFindSessionsComplete);
			bIsSearching = true;
			Session->FindSessions(0, SessionSearch.ToSharedRef());
		}
	}
}

void USubmarineGameInstance::JoinSession(int SessionNumber)
{
	if (bIsSearching || SessionSearch->SearchResults.Num() < 1)
	{
		UE_LOG(LogTemp, Warning, TEXT("No sessions to join!"))
		return;
	}

	if (OnlineSubsystem)
	{
		if (const IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface())
		{
			if (SessionSearch->SearchResults.Num() > 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("%d sessions found!"), SessionSearch->SearchResults.Num())
			}
			Session->OnJoinSessionCompleteDelegates.AddUObject(this, &USubmarineGameInstance::OnJoinSessionComplete);
			Session->JoinSession(0, TestSessionName, SessionSearch->SearchResults[SessionNumber]);
		}
		
	}
}

TArray<FSubmarineSession> USubmarineGameInstance::GetSearchResults()
{
	TArray<FSubmarineSession> Results = TArray<FSubmarineSession>();
	for (int i = 0; i < SessionSearch->SearchResults.Num(); ++i)
	{
		FSubmarineSession SubmarineSession = FSubmarineSession();
		const auto& SearchResult = SessionSearch->SearchResults[i];
		const IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
		// TODO: Figure out how to Set/Retrieve a custom Session Name
		// const NamedSession = SessionInterface->GetNamedSession()
		// 
		// if (NamedSession)
		// {
		// 	SubmarineSession.Name = NamedSession->SessionName.ToString();
		// }
		// else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to fetch Session name."))
			SubmarineSession.Name = SearchResult.GetSessionIdStr();
		}
		SubmarineSession.NumPlayers = 12 - SearchResult.Session.NumOpenPrivateConnections;
		SubmarineSession.ListIndex = i;
		Results.Add(SubmarineSession);
	}
	return Results;
}


void USubmarineGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("Create Session %s Succeeded: %d"), *SessionName.ToString(), bWasSuccessful);

	if (GEngine && !bWasSuccessful)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red,
			TEXT("Failed to Create Session. You may want to restart the game before trying again."));
	}
	
	if (!OnlineSubsystem)
	{
		LogNoSubsystem();
		return;
	}
	if (const IOnlineSessionPtr Session = OnlineSubsystem->GetSessionInterface())
	{
		Session->ClearOnCreateSessionCompleteDelegates(this);
	}
	else
	{
		LogNoSessionInterface();
	}

	CreateSessionsCompleted.Broadcast(bWasSuccessful);
}

